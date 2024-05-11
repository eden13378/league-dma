#pragma once

namespace lua {
    class SdkDownloader {
    public:
        auto download_sdk( ) -> void;

    private:
        static auto clone_sdk( std::string git_path ) -> void;

        static auto        check_git_default_location( ) -> bool;
        static auto check_git_in_path( ) -> bool;

        static auto exec( std::string command, const std::optional< std::string >& working_dir ) -> bool;
    };
}
