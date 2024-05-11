#pragma once

namespace sdk::game {
    class BuffInfo {
    public:
        auto get_name( ) -> std::string{
            std::array< char, 84 > v{ };

            const auto ptr = reinterpret_cast< intptr_t >( name );
            if ( ptr == 0 ) return { };

            app->memory->read_amount( ptr, v.data( ), 84 );

            return std::string(v.data(  ));
        }

    private:
        char  pad_0000[ 8 ]; //0x0000
        char* name; //0x0008
    };
}
