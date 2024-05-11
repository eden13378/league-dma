#include "pch.hpp"

#include "loader.hpp"

#pragma optimize( "", off )
void ModuleMapper::read_memory( uintptr_t address, void *buffer, size_t len ) {
    memcpy( buffer, ( void * )address, len );
}

void ModuleMapper::write_memory( uintptr_t address, void *buffer, size_t len ) {
    memcpy( ( void * )address, buffer, len );
}

PIMAGE_SECTION_HEADER ModuleMapper::get_section_header_rva( uint32_t rva ) {
    auto current_section = IMAGE_FIRST_SECTION( this->pe_image_nt );
    for ( int i = 0; i < this->pe_image_nt->FileHeader.NumberOfSections; i++ ) {
        if ( rva >= current_section->VirtualAddress &&
             rva < current_section->VirtualAddress + current_section->SizeOfRawData ) {
            return current_section;
        }
        current_section++;
    }
    return nullptr;
}

uintptr_t ModuleMapper::rva_to_file_offset( uint32_t rva ) {
    auto section_header = get_section_header_rva( rva );
    if ( !section_header ) { return 0; }

    auto file_offset = ( rva - section_header->VirtualAddress + section_header->PointerToRawData );

    return ( uintptr_t )( this->pe_image ) + file_offset;
}

void *ModuleMapper::allocate_private_executable( size_t size ) {
    return VirtualAlloc( NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
}

bool ModuleMapper::allocate_image( ) {
    auto allocation_base = ( uintptr_t )allocate_private_executable( this->region_size );
    if ( allocation_base ) { this->image_base = allocation_base; }

    return this->image_base != 0;
}

bool ModuleMapper::map_sections( ) {
    auto current_section = IMAGE_FIRST_SECTION( this->pe_image_nt );
    if ( !current_section ) { return false; }

    for ( int i = 0; i < this->pe_image_nt->FileHeader.NumberOfSections; i++ ) {
        if ( current_section->Characteristics & ( IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE ) ) {
            if ( current_section->SizeOfRawData > 0 ) {
                auto dst_addr = this->image_base + current_section->VirtualAddress;
                auto src_addr = this->pe_image + current_section->PointerToRawData;

                write_memory( dst_addr, ( void * )src_addr, current_section->SizeOfRawData );
            }
        }

        current_section++;
    }
    return true;
}

bool ModuleMapper::process_relocs( ) {
    int  counter   = 0;
    auto reloc_dir = this->pe_image_nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ];

    if ( reloc_dir.VirtualAddress != 0 && reloc_dir.Size > 0 ) {
        auto delta = ( intptr_t )this->image_base - ( intptr_t )this->pe_image_nt->OptionalHeader.ImageBase;

        if ( delta == 0 ) { return true; }

        auto reloc = ( PIMAGE_BASE_RELOCATION )( rva_to_file_offset( reloc_dir.VirtualAddress ) );
        if ( !reloc ) { return false; }

        auto c = 0u;
        while ( c < reloc_dir.Size ) {
            size_t    p      = sizeof( IMAGE_BASE_RELOCATION );
            uint16_t *chains = ( uint16_t * )( ( PUCHAR )reloc + p );
            if ( reloc->SizeOfBlock == 0 ) { return false; }
            while ( p < reloc->SizeOfBlock ) {
                uintptr_t Base = rva_to_file_offset( reloc->VirtualAddress );
                auto      ptr  = Base + ( *chains & 0xFFF );
                switch ( *chains >> 12 ) {
                case IMAGE_REL_BASED_HIGHLOW:
                {
                    auto old_val          = *( int32_t          *)( ptr );
                    auto new_val          = old_val + ( int32_t )delta;
                    *( int32_t * )( ptr ) = new_val;
                    break;
                }
                case IMAGE_REL_BASED_DIR64:
                {
                    auto old_val          = *( int64_t          *)( ptr );
                    auto new_val          = old_val + ( int64_t )delta;
                    *( int64_t * )( ptr ) = new_val;
                    break;
                }
                case IMAGE_REL_BASED_ABSOLUTE:
                {
                    break;
                }
                default:
                {
                    break;
                }
                }
                chains++;
                p += sizeof( WORD );
            }
            c += reloc->SizeOfBlock;
            reloc = ( PIMAGE_BASE_RELOCATION )( ( uint8_t * )reloc + reloc->SizeOfBlock );
        }
    }
    return true;
}

bool ModuleMapper::resolve_imports( ) {
    IMAGE_DATA_DIRECTORY importDir = this->pe_image_nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ];
    if ( !importDir.VirtualAddress || !importDir.Size ) { return true; }

    auto import_table = ( PIMAGE_IMPORT_DESCRIPTOR )( rva_to_file_offset( importDir.VirtualAddress ) );

    while ( import_table->Name ) {
        auto lib_name = ( char * )( rva_to_file_offset( import_table->Name ) );

        auto library_base = ( uintptr_t )GetModuleHandleA( lib_name );

        if ( !library_base ) { return false; }

        PIMAGE_THUNK_DATA thunk = nullptr;
        if ( import_table->OriginalFirstThunk == 0 ) {
            thunk = ( PIMAGE_THUNK_DATA )( rva_to_file_offset( import_table->FirstThunk ) );
        } else {
            thunk = ( PIMAGE_THUNK_DATA )( rva_to_file_offset( import_table->OriginalFirstThunk ) );
        }
        int import_index = 0;
        while ( thunk->u1.Function ) {
            uintptr_t function_addr = 0;

            if ( ( thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64 ) == 0 ) {
                auto name_import = ( PIMAGE_IMPORT_BY_NAME )( rva_to_file_offset( thunk->u1.AddressOfData ) );
                function_addr    = ( uintptr_t )GetProcAddress( ( HMODULE )library_base, ( name_import->Name ) );

            } else {
                function_addr =
                    ( uintptr_t )GetProcAddress( ( HMODULE )library_base, ( LPCSTR )( thunk->u1.Ordinal & 0xFFFF ) );
            }

            if ( !function_addr ) { return false; }

            auto imp_thunk_addr =
                this->image_base + ( import_table->FirstThunk ) + ( sizeof( uintptr_t ) * import_index );
            write( imp_thunk_addr, function_addr );

            thunk++;
            import_index++;
        }
        import_table++;
    }
    return true;
}

bool ModuleMapper::load( void *pe_image, size_t pe_image_len ) {
    if ( !pe_image || !pe_image_len ) { return false; }

    this->pe_image_len = pe_image_len;

    this->pe_image_dos = reinterpret_cast< PIMAGE_DOS_HEADER >( pe_image );
    if ( this->pe_image_dos->e_magic != IMAGE_DOS_SIGNATURE ) { return false; }

    this->pe_image_nt = reinterpret_cast< PIMAGE_NT_HEADERS >( ( uintptr_t )pe_image + this->pe_image_dos->e_lfanew );
    if ( this->pe_image_nt->Signature != IMAGE_NT_SIGNATURE ) { return false; }

    this->region_size = this->pe_image_nt->OptionalHeader.SizeOfImage;

    this->pe_image = ( uintptr_t )malloc( pe_image_len );
    if ( !this->pe_image ) { return false; }

    memcpy( ( void * )this->pe_image, pe_image, pe_image_len );

    if ( !allocate_image( ) ) {
        free( ( void * )this->pe_image );
        return false;
    }

    if ( !process_relocs( ) ) {
        free( ( void * )this->pe_image );
        return false;
    }

    if ( !map_sections( ) ) {
        free( ( void * )this->pe_image );
        return false;
    }

    if ( !resolve_imports( ) ) {
        free( ( void * )this->pe_image );
        return false;
    }

    free( ( void * )this->pe_image );
    this->pe_image     = 0;
    this->pe_image_dos = nullptr;
    return true;
}

int ModuleMapper::execute_entrypoint( HMODULE dll, DWORD reason, LPVOID reserved ) {
    int64_t   entry_point_rva = this->pe_image_nt->OptionalHeader.AddressOfEntryPoint;
    uintptr_t entry_point_va  = ( uintptr_t )( this->image_base ) + entry_point_rva;

    using DllMain_t = int ( * )( HMODULE, DWORD, LPVOID );
    return ( ( DllMain_t )entry_point_va )( dll, reason, reserved );
}
#pragma optimize( "", on )