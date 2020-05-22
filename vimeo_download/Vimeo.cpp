//
//  VimeoParse.cpp
//  vimeo_download
//
//  Created by MitsukiGoto on 2020/05/16.
//  Copyright © 2020 MitsukiGoto. All rights reserved.
//

#include "Vimeo.hpp"
#include "Requests.hpp"
#include "Util/cppcodec/base64_rfc4648.hpp"
#include <regex>
#include <tuple>
#include <filesystem>
#include <future>
#include <array>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>

Vimeo::Vimeo(const std::string& oupput_name, const std::string& url, std::unique_ptr<JSON> json, bool verbose) : output_name(oupput_name),url(url),verbose(verbose) {
    this->json = std::move(json);
    this->base_url = std::regex_replace(url, std::regex(R"(sep/.+)"), "");
    this->tmp_dir = std::string(std::filesystem::temp_directory_path());
}

void Vimeo::merge() {
#ifdef __APPLE__
    std::string command = "ffmpeg -i " + this->tmp_dir + "a.mp3 " + "-i " + this->tmp_dir + "v.mp4" + " -acodec" + " copy" + " -vcodec" + " copy" + " /Users/mitsukigoto/Desktop/Vimeo/" + this->output_name + " >/dev/null 2>&1";
    if(this->verbose) {
        command = "ffmpeg -i " + this->tmp_dir + "a.mp3 " + "-i " + this->tmp_dir + "v.mp4" + " -acodec" + " copy" + " -vcodec" + " copy" + " /Users/mitsukigoto/Desktop/Vimeo/" + this->output_name;
        std::cout << command << std::endl;
    }
    std::system(command.c_str());
    std::cout << "\u001b[35m" << "Merging Audio and Video has successflly done" << std::endl;
    std::cout << "The Video was saved at:" << " /Users/mitsukigoto/Desktop/Vimeo/" << this->output_name << "\u001b[0m" << std::endl;
#endif
#ifdef __CYGWIN__
    std::cout << "Sorry, Merging Audio and Video doesn't support on Windows now" << std::endl;
#endif
}

Vimeo& Vimeo::download() {
    auto process1 = std::thread([this]{this->downloadVideo();});
    auto process2 = std::thread([this]{this->downloadAudio();});
    process1.join();
    process2.join();
    return *this;
}

void Vimeo::downloadVideo() {
    auto video = json->v.get<picojson::object>()["video"].get<picojson::array>();
    int i = 0;
    std::vector<std::tuple<int, int>> heights;
    for(auto& content:video) {
        std::string height = content.get<picojson::object>()["height"].to_str();
        std::tuple<int, int> t = {std::stoi(height), i};
        heights.push_back(t);
        i++;
    }
    std::sort(std::begin(heights), std::end(heights), std::greater<std::tuple<int, int>>());
    picojson::object highest_video = video[std::get<1>(heights[0])].get<picojson::object>();
    std::string video_base_url = this->base_url + "sep/video/" + highest_video.at("base_url").to_str();
    if(this->verbose){
        std::cout << "video_base_url: " << video_base_url <<  std::endl;
    }
    auto decoded = cppcodec::base64_rfc4648::decode(highest_video.at("init_segment").to_str());
    std::ofstream ofs(this->tmp_dir+"v.mp4", std::ios::out|std::ios::binary);
    std::stringstream s;
    std::move(decoded.begin(), decoded.end(), std::ostream_iterator<unsigned char>(s));
    ofs << s.rdbuf();
    ofs.close();
    for(auto& segment:highest_video.at("segments").get<picojson::array>()) {
        std::string segment_url = video_base_url+segment.get<picojson::object>()["url"].to_str();
        Requests::get(segment_url, this->tmp_dir+"v.mp4");
    }
    std::cout << "\u001b[36m" << "Downloading Video has successflly done" << "\u001b[0m" << std::endl;
    if(this->verbose){
        std::cout << "Only Video at: " << this->tmp_dir+"v.mp4" <<  std::endl;
    }
}

void Vimeo::downloadAudio() {
    auto audio = json->v.get<picojson::object>()["audio"].get<picojson::array>()[0].get<picojson::object>();
    std::string audio_base_url = this->base_url + "sep/audio/" + audio.at("base_url").to_str().erase(0, 9);
    if(this->verbose){
        std::cout << "audio_base_url: " << audio_base_url <<  std::endl;
    }
    auto decoded = cppcodec::base64_rfc4648::decode(audio.at("init_segment").to_str());
    std::ofstream ofs(this->tmp_dir+"a.mp3", std::ios::out|std::ios::binary);
    std::stringstream s;
    std::move(decoded.begin(), decoded.end(), std::ostream_iterator<unsigned char>(s));
    ofs << s.rdbuf();
    ofs.close();
    for(auto& segment:audio.at("segments").get<picojson::array>()) {
        std::string segment_url = audio_base_url+segment.get<picojson::object>()["url"].to_str();
        Requests::get(segment_url, this->tmp_dir+"a.mp3");
    }
    std::cout << "\u001b[32m" << "Downloading Audio has successflly done" << "\u001b[0m" << std::endl;
    if(this->verbose){
        std::cout << "Only Audio at: " << this->tmp_dir+"v.mp4" <<  std::endl;
    }
}
