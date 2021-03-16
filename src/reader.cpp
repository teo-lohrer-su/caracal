#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <tins/tins.h>

#include <dminer/parser.hpp>
#include <dminer/reader.hpp>
#include <dminer/statistics.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace dminer::Reader {

Statistics::Sniffer read(const fs::path& input_file,
                         const fs::path& output_file, const std::string& round,
                         const bool include_rtt) {
  std::ofstream output_csv{output_file};
  Statistics::Sniffer statistics{};
  Tins::FileSniffer sniffer{input_file};

  auto handler = [&output_csv, &statistics, round,
                  include_rtt](Tins::Packet& packet) {
    auto reply = Parser::parse(packet);

    if (statistics.received_count % 1'000'000 == 0) {
      spdlog::info(statistics);
    }

    if (reply) {
      statistics.icmp_messages_all.insert(reply->src_ip);
      if (IN6_ARE_ADDR_EQUAL(&reply->src_ip, &reply->inner_dst_ip)) {
        statistics.icmp_messages_path.insert(reply->src_ip);
      }
      output_csv << fmt::format("{},{},{}\n", reply->to_csv(include_rtt), round,
                                "1");
    }

    statistics.received_count++;
    return true;
  };

  sniffer.sniff_loop(handler);
  spdlog::info(statistics);
  return statistics;
}

}  // namespace dminer::Reader
