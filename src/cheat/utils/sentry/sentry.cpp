#include "pch.hpp"

#include "sentry.hpp"

#include <sentry.h>

namespace utils {
    Sentry::ContextObjectBuilder::ContextObjectBuilder( ){ m_object = sentry_value_new_object( ); }

    Sentry::~Sentry( ){ sentry_shutdown( ); }

    auto Sentry::add_attachment( const std::string& path ) -> void{
        std::lock_guard lock( m_mutex );

        sentry_options_add_attachment(
            m_options,
            path.c_str( )
        );
    }

    auto Sentry::track_exception( const std::string& message ) -> void{
        std::lock_guard lock( m_mutex );

        const auto event = sentry_value_new_event( );

        const auto exception = sentry_value_new_exception( "Exception", message.c_str( ) );
        sentry_value_set_stacktrace( exception, nullptr, 0 );
        sentry_event_add_exception( event, exception );

        sentry_capture_event( event );
    }

    auto Sentry::add_message(
        const std::string&   logger,
        const std::string&   message,
        const sentry_level_e level
    ) -> void{
        std::lock_guard lock( m_mutex );

        sentry_value_t event = sentry_value_new_message_event(
            level,
            logger.c_str( ),
            message.c_str( )
        );

        sentry_capture_event( event );
    }

    auto Sentry::add_context_object(
        const std::string&                                            object_name,
        const std::function< void( ContextObjectBuilder* builder ) >& cb
    ) -> void{
        std::lock_guard lock( m_mutex );

        ContextObjectBuilder builder;
        cb( &builder );

        sentry_set_context( object_name.c_str( ), builder.build( ) );
    }

    auto Sentry::remove_context_object( const std::string& object_name ) -> void{
        sentry_remove_context( object_name.c_str( ) );
    }

    auto Sentry::add_breadcrumb_object(
        const std::string&                                         message,
        const std::function< void( BreadcrumbBuilder* builder ) >& cb,
        const std::string&                                         type
    ) -> void{
        std::lock_guard lock( m_mutex );

        BreadcrumbBuilder builder( message, type );
        cb( &builder );

        sentry_add_breadcrumb( builder.build( ) );
    }

    auto Sentry::identify_user(
        const std::optional< std::string >& username,
        const std::optional< int32_t >      id,
        const std::optional< std::string >& email,
        const std::optional< std::string >& ip_address
    ) -> void{
        std::lock_guard lock( m_mutex );

        const sentry_value_t user = sentry_value_new_object( );
        if ( ip_address ) {
            sentry_value_set_by_key(
                user,
                "ip_address",
                sentry_value_new_string( ip_address->c_str( ) )
            );
        }
        if ( email ) {
            sentry_value_set_by_key(
                user,
                "email",
                sentry_value_new_string( email->c_str( ) )
            );
        }
        if ( username ) {
            sentry_value_set_by_key(
                user,
                "username",
                sentry_value_new_string( username->c_str( ) )
            );
        }
        if ( id ) {
            sentry_value_set_by_key(
                user,
                "id",
                sentry_value_new_int32( *id )
            );
        }

        sentry_set_user( user );
    }

    auto Sentry::reset_user( ) -> void{
        std::lock_guard lock( m_mutex );
        sentry_remove_user( );
    }

    auto Sentry::set_tag( const std::string& key, const std::string& value ) -> void{
        std::lock_guard lock( m_mutex );
        sentry_set_tag( key.c_str( ), value.c_str( ) );
    }

    auto Sentry::remove_tag( const std::string& key ) -> void{
        std::lock_guard lock( m_mutex );
        sentry_remove_tag( key.c_str( ) );
    }

    auto Sentry::start_transaction(
        const std::string& name,
        const std::string& operation
    ) -> std::shared_ptr< Transaction >{
        std::lock_guard lock( m_mutex );
        return std::make_shared< Transaction >( name, operation );
    }

    auto Sentry::add_breadcrumb(
        const std::string&      message,
        const std::string&      category,
        const EBreadcrumbsLevel level
    ) -> void{
        std::lock_guard lock( m_mutex );

        const auto sentry_level = []( const EBreadcrumbsLevel report_level ) -> std::string{
            switch ( report_level ) {
            case EBreadcrumbsLevel::fatal:
                return _( "fatal" );
            case EBreadcrumbsLevel::error:
                return _( "error" );
            case EBreadcrumbsLevel::warning:
                return _( "warning" );
            case EBreadcrumbsLevel::info:
                return _( "info" );
            case EBreadcrumbsLevel::debug:
                return _( "debug" );
            default:
                return _( "info" );
            }
        };

        const auto lvl = sentry_level( level );

        const sentry_value_t crumb = sentry_value_new_breadcrumb( "default", message.data( ) );
        sentry_value_set_by_key( crumb, "category", sentry_value_new_string( category.data( ) ) );
        sentry_value_set_by_key( crumb, "level", sentry_value_new_string( lvl.data( ) ) );
        sentry_add_breadcrumb( crumb );
    }

    auto Sentry::shutdown( ) -> void{
        std::lock_guard lock( m_mutex );
        sentry_shutdown( );
    }

    auto Sentry::ContextObjectBuilder::add_int( const std::string& key, const int32_t value ) const -> void{
        sentry_value_set_by_key( m_object, key.c_str( ), sentry_value_new_int32( value ) );
    }

    auto Sentry::ContextObjectBuilder::add_bool( const std::string& key, const bool value ) -> void{
        sentry_value_set_by_key( m_object, key.c_str( ), sentry_value_new_bool( value ) );
    }

    auto Sentry::ContextObjectBuilder::add_double(
        const std::string& key,
        const double       value
    ) -> void{ sentry_value_set_by_key( m_object, key.c_str( ), sentry_value_new_double( value ) ); }

    auto Sentry::ContextObjectBuilder::add_string(
        const std::string& key,
        const std::string& value
    ) -> void{ sentry_value_set_by_key( m_object, key.c_str( ), sentry_value_new_string( value.c_str( ) ) ); }

    auto Sentry::ContextObjectBuilder::build( ) const -> sentry_value_t{ return m_object; }

    Sentry::BreadcrumbBuilder::BreadcrumbBuilder( std::string message, std::string type ){
        m_value = sentry_value_new_breadcrumb( type.c_str( ), message.c_str( ) );
    }

    auto Sentry::BreadcrumbBuilder::add_int( const std::string& key, const int32_t value ) -> BreadcrumbBuilder*{
        sentry_value_set_by_key( m_value, key.c_str( ), sentry_value_new_int32( value ) );
        return this;
    }

    auto Sentry::BreadcrumbBuilder::add_string(
        const std::string& key,
        const std::string& value
    ) -> BreadcrumbBuilder*{
        sentry_value_set_by_key( m_value, key.c_str( ), sentry_value_new_string( value.c_str( ) ) );
        return this;
    }

    auto Sentry::BreadcrumbBuilder::build( ) const -> sentry_value_t{ return m_value; }

    auto Sentry::Transaction::ChildTransaction::finish( ) const -> void{ sentry_span_finish( m_span ); }

    Sentry::Transaction::Transaction( std::string name, std::string operation ){
        sentry_transaction_context_t* tx_ctx = sentry_transaction_context_new(
            name.data( ),
            operation.data( )
        );
        m_tx = sentry_transaction_start( tx_ctx, sentry_value_new_null( ) );
    }

    auto Sentry::Transaction::finish( ) const -> void{ sentry_transaction_finish( m_tx ); }

    auto Sentry::Transaction::add_child(
        std::string name,
        std::string operation
    ) -> std::shared_ptr< ChildTransaction >{ return std::make_shared< ChildTransaction >( m_tx, name, operation ); }

    auto Sentry::BreadcrumbBuilder::add_bool( const std::string& key, const bool value ) -> BreadcrumbBuilder*{
        sentry_value_set_by_key( m_value, key.c_str( ), sentry_value_new_bool( value ) );
        return this;
    }

    auto Sentry::BreadcrumbBuilder::add_double( const std::string& key, const double value ) -> BreadcrumbBuilder*{
        sentry_value_set_by_key( m_value, key.c_str( ), sentry_value_new_double( value ) );
        return this;
    }

    Sentry::Sentry( const std::string& dns, const std::string& version ): m_version( version ), m_dns{ dns }{
        std::lock_guard lock( m_mutex );

        char buffer[ MAX_PATH ];
        GetModuleFileNameA( nullptr, buffer, MAX_PATH );
        const std::string::size_type pos = std::string( buffer ).find_last_of( _( "\\/" ) );

        const auto crash_handler_path = ( std::string( buffer ).substr( 0, pos ) +
            std::string( _( "\\chandler.exe" ) ) );

        m_options = sentry_options_new( );
        sentry_options_set_dsn(
            m_options,
            std::string(
                _( "https://b541cd3b6574445c88e1fd09956c0985@o1321659.ingest.sentry.io/6578338" )
            ).c_str( )
        );
        sentry_options_set_environment( m_options, std::string( _( "production" ) ).c_str( ) );
        sentry_options_set_handler_path( m_options, crash_handler_path.data( ) );

        sentry_options_set_release( m_options, version.c_str( ) );

        sentry_options_set_sample_rate( m_options, 1.0 );
        sentry_options_set_symbolize_stacktraces( m_options, 0 );
        sentry_options_set_traces_sample_rate( m_options, 1.0 );
        sentry_options_set_system_crash_reporter_enabled( m_options, false );
        sentry_options_set_max_breadcrumbs( m_options, 500 );
        sentry_options_set_max_spans( m_options, 500 );
        sentry_options_set_require_user_consent( m_options, 0 );
        sentry_options_set_shutdown_timeout( m_options, 1000 * 5 );
        sentry_options_set_auto_session_tracking( m_options, 1 );

        sentry_init( m_options );

        sentry_user_consent_give( );

        sentry_set_level( SENTRY_LEVEL_INFO );

        sentry_set_transaction( "init" );

        sentry_start_session( );
    }
}
