#pragma once
#include <mutex>

namespace utils {
    class Sentry final {
    public:
        enum class EBreadcrumbsLevel {
            fatal,
            error,
            warning,
            info,
            debug
        };

        enum class EErrorLevel {
            debug   = -1,
            info    = 0,
            warning = 1,
            error   = 2,
            fatal   = 3,
        };

        class ContextObjectBuilder {
        public:
            ContextObjectBuilder( );

            auto add_int( const std::string& key, int32_t value ) const -> void;
            auto add_bool( const std::string& key, bool value ) -> void;
            auto add_double( const std::string& key, double value ) -> void;
            auto add_string( const std::string& key, const std::string& value ) -> void;

            [[nodiscard]] auto build( ) const -> sentry_value_t;

        private:
            sentry_value_t m_object;
        };

        class BreadcrumbBuilder {
        public:
            explicit BreadcrumbBuilder( std::string message, std::string type = "default" );

            auto add_int( const std::string& key, int32_t value ) -> BreadcrumbBuilder*;
            auto add_bool( const std::string& key, bool value ) -> BreadcrumbBuilder*;
            auto add_double( const std::string& key, double value ) -> BreadcrumbBuilder*;
            auto add_string( const std::string& key, const std::string& value ) -> BreadcrumbBuilder*;

            [[nodiscard]] auto build( ) const -> sentry_value_t;

        private:
            sentry_value_t m_value;
        };

        class Transaction {
            class ChildTransaction {
            public:
                ChildTransaction( sentry_transaction_t* tx, std::string name, std::string operation ){
                    m_span = sentry_transaction_start_child(
                        tx,
                        name.data( ),
                        operation.data( )
                    );
                }

                auto finish( ) const -> void;

            private:
                sentry_span_t* m_span{ };
            };

        public:
            Transaction( std::string name, std::string operation );

            auto finish( ) const -> void;

            auto add_child( std::string name, std::string operation ) -> std::shared_ptr< ChildTransaction >;

        private:
            sentry_transaction_t* m_tx{ };
        };

    public:
        explicit Sentry( const std::string& dns, const std::string& version );

        ~Sentry( );

        auto add_attachment( const std::string& path ) -> void;

        auto track_exception( const std::string& message ) -> void;

        auto add_message(
            const std::string& logger,
            const std::string& message,
            sentry_level_e     level = sentry_level_e::SENTRY_LEVEL_INFO
        ) -> void;

        auto add_context_object(
            const std::string&                                            object_name,
            const std::function< void( ContextObjectBuilder* builder ) >& cb
        ) -> void;

        auto remove_context_object( const std::string& object_name ) -> void;

        auto add_breadcrumb_object(
            const std::string&                                         message,
            const std::function< void( BreadcrumbBuilder* builder ) >& cb,
            const std::string&                                         type = "default"
        ) -> void;

        auto identify_user(
            const std::optional< std::string >& username,
            std::optional< int32_t >            id,
            const std::optional< std::string >& email,
            const std::optional< std::string >& ip_address
        ) -> void;

        auto reset_user( ) -> void;
        auto set_tag( const std::string& key, const std::string& value ) -> void;
        auto remove_tag( const std::string& key ) -> void;

        auto start_transaction(
            const std::string& name,
            const std::string& operation
        ) -> std::shared_ptr< Transaction >;

        auto add_breadcrumb(
            const std::string& message,
            const std::string& category,
            EBreadcrumbsLevel  level
        ) -> void;

        auto shutdown( ) -> void;

    private:
        std::string       m_version{ };
        std::string       m_dns{ };
        sentry_options_t* m_options{ };
        std::mutex        m_mutex;
    };
}
