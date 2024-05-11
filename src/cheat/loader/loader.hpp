#pragma once

#include <cstdint>
#include <functional>
#include <windows.h>

#pragma optimize( "", off )
class ModuleMapper {
public:
    uintptr_t         pe_image     = 0;
    size_t            pe_image_len = 0;
    PIMAGE_NT_HEADERS pe_image_nt  = nullptr;
    PIMAGE_DOS_HEADER pe_image_dos = nullptr;
    uintptr_t         image_base   = 0;
    size_t            region_size  = 0;

    template < class T >
    T read( uintptr_t address ) {
        T      Val{ };
        SIZE_T read = 0;
        read_memory( address, ( void * )&Val, sizeof( T ) );
        return Val;
    }

    template < class T >
    void write( uintptr_t address, const T &Val ) {
        write_memory( address, ( void * )&Val, sizeof( T ) );
    }

    bool load( void *pe_image, size_t pe_image_len );
    int  execute_entrypoint( HMODULE dll, DWORD reason, LPVOID reserved );

private:
    bool resolve_imports( );
    void read_memory( uintptr_t address, void *buffer, size_t len );
    void write_memory( uintptr_t address, void *buffer, size_t len );
    bool allocate_image( );

    bool map_sections( );

    bool process_relocs( );

    PIMAGE_SECTION_HEADER get_section_header_rva( uint32_t rva );
    uintptr_t             rva_to_file_offset( uint32_t rva );
    void                 *allocate_private_executable( size_t size );
};

struct Injector {
    std::function< int( int process_id, uint64_t *shared_address ) > inject;
};

#pragma optimize( "", on )