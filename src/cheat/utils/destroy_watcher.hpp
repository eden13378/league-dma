#pragma once

#if __RESHARPER__
#define rscpp_guard [[rscpp::guard]]
#else
#define rscpp_guard 
#endif

namespace utils {
    class rscpp_guard DestroyWatcher {
    public:
        explicit DestroyWatcher( std::function< void( ) > fn ){ m_callback = std::move( fn ); }

        ~DestroyWatcher( ){ m_callback( ); }

    private:
        std::function< void( ) > m_callback;
    };
}
