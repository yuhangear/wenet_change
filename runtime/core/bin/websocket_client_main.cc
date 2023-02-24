// Copyright (c) 2020 Mobvoi Inc (Binbin Zhang)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "frontend/wav2.h"
#include "utils/flags.h"
#include "utils/timer.h"
#include "websocket/websocket_client.h"

DEFINE_string(hostname, "127.0.0.1", "hostname of websocket server");
DEFINE_int32(port, 10086, "port of websocket server");
DEFINE_int32(nbest, 1, "n-best of decode result");
DEFINE_int32(hot_list, 0, "第几个热词列表");
DEFINE_int32(sample_rate, 16000, "采样率");
DEFINE_string(wav_path, "", "test wav file path");
DEFINE_bool(continuous_decoding, false, "continuous decoding mode");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  wenet::WebSocketClient client(FLAGS_hostname, FLAGS_port);
  client.set_nbest(FLAGS_nbest);
  client.set_hot_list(FLAGS_hot_list);
  client.set_sample_rate(FLAGS_sample_rate);
  client.set_continuous_decoding(FLAGS_continuous_decoding);
  client.SendStartSignal();

  wenet::WavReader wav_reader(FLAGS_wav_path);
  const int sample_rate = wav_reader.sample_rate();
  // // Only support 16K
  // CHECK_EQ(wav_reader.sample_rate(), sample_rate);


  int num_samples = wav_reader.num_samples();

  // if(sample_rate==16000){
  //   int bias = (num_samples%8000)%2;
  //   num_samples=num_samples-bias;
  // }
  // else{
  //   int bias = (num_samples%4000)%2;
  //   if(bias==0){
  //     bias=1;
  //   }else{
  //     bias=0;
  //   }
  //   num_samples=num_samples-bias;
  // }

  // Send data every 0.5 second 640
  const float interval = 0.5;
  const int sample_interval = interval * sample_rate;
  for (int start = 0; start < num_samples; start += sample_interval) {
    if (client.done()) {
      break;
    }
    int end = std::min(start + sample_interval, num_samples);
    // Convert to short
    std::vector<int16_t> data;
    data.reserve(end - start);
    for (int j = start; j < end; j++) {
      data.push_back(static_cast<int16_t>(wav_reader.data()[j]));
    }
    // TODO(Binbin Zhang): Network order?
    // Send PCM data

    client.SendBinaryData(data.data(), data.size() * sizeof(int16_t));
    VLOG(2) << "Send " << data.size() << " samples";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(interval * 1000)));
    
  }
  wenet::Timer timer;
  client.SendEndSignal();
  client.Join();
  VLOG(2) << "Total latency: " << timer.Elapsed() << "ms.";
  return 0;
}
