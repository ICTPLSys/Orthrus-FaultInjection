#include <boost/program_options.hpp>
#include "load_file.hpp"

struct Args {
    std::string primary_ip;
    size_t primary_port;
};

Args parse_args(int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("primary-ip,ipp", po::value<std::string>()->default_value("localhost"),
         "primary ip listen to loader")
        ("primary-port,pp", po::value<size_t>()->default_value(12221),
         "primary port listen to loader");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    return Args{vm["primary-ip"].as<std::string>(),
                vm["primary-port"].as<size_t>()};
}

constexpr size_t kPacketSize = 1 << 15;

int main(int argc, char **argv) {
    auto args = parse_args(argc, argv);
    auto start_load_time = rdtsc();
    const char buffer[] = {
        "five seven three six five four five seven seven five three six seven "
        "six six seven two seven two four four three five six six four seven "
        "one ten nine seven six five four three two one six seven five four "
        "three two one six seven five four three two one six seven five four "
        "three two one six seven five four three two two one one nine eight "
    };
    size_t file_size = std::strlen(buffer);
    auto end_load_time = rdtsc();
    printf("Load time: %zu ms\n",
           microsecond(start_load_time, end_load_time) / 1000);

    int fd = connect_server(args.primary_ip, args.primary_port);
    if (fd < 0) {
        std::cerr << "Error: failed to connect to primary" << std::endl;
        exit(1);
    }

    char *wt_buffer = (char *)malloc(kPacketSize + 50);
    sprintf(wt_buffer, "%zu\x01", file_size);
    write_all(fd, wt_buffer, strlen(wt_buffer));
    for (size_t i = 0; i < file_size; i += kPacketSize) {
        size_t len = std::min(kPacketSize, file_size - i);
        memset(wt_buffer, '0', 15);
        for (size_t k = i, p = 14; k; k /= 10, p--) wt_buffer[p] = '0' + k % 10;
        memcpy(wt_buffer + 15, buffer + i, len);
        wt_buffer[15 + len] = '\x01';
        wt_buffer[15 + len + 1] = '\0';
        write_all(fd, wt_buffer, len + 16);
    }
    close(fd);
}
