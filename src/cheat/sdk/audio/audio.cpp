#include "pch.hpp"

#include "audio.hpp"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

namespace sdk::audio {
    static IXAudio2*               m_xaudio2;
    static IXAudio2MasteringVoice* m_master_voice;

    auto initialize( ) -> void{
        auto result = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
        if ( FAILED( result ) ) throw std::runtime_error( "Failed to initialize COM" );

        m_xaudio2 = nullptr;
        if ( FAILED( result = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR) ) ) throw std::runtime_error( "Failed to initialize XAudio" );

        m_master_voice = nullptr;
        if ( FAILED( result = m_xaudio2->CreateMasteringVoice(&m_master_voice) ) ) throw std::runtime_error( "Failed to create mater voice" );
    }

    HRESULT find_chunk( const HANDLE h_file, const DWORD fourcc, DWORD& dw_chunk_size, DWORD& dw_chunk_data_position ){
        HRESULT hr = S_OK;
        if ( INVALID_SET_FILE_POINTER == SetFilePointer( h_file, 0, nullptr, FILE_BEGIN ) ) return HRESULT_FROM_WIN32( GetLastError( ) );

        DWORD       dw_chunk_type;
        DWORD       dw_chunk_data_size;
        DWORD       dw_riff_data_size = 0;
        DWORD       dw_file_type;
        const DWORD bytes_read = 0;
        DWORD       dw_offset  = 0;

        while ( hr == S_OK ) {
            DWORD dwRead;
            if ( 0 == ReadFile( h_file, &dw_chunk_type, sizeof( DWORD ), &dwRead, nullptr ) ) hr = HRESULT_FROM_WIN32( GetLastError( ) );

            if ( 0 == ReadFile( h_file, &dw_chunk_data_size, sizeof( DWORD ), &dwRead, nullptr ) ) hr = HRESULT_FROM_WIN32( GetLastError( ) );

            switch ( dw_chunk_type ) {
            case fourccRIFF:
                dw_riff_data_size = dw_chunk_data_size;
                dw_chunk_data_size = 4;
                if ( 0 == ReadFile( h_file, &dw_file_type, sizeof( DWORD ), &dwRead, nullptr ) ) hr = HRESULT_FROM_WIN32( GetLastError( ) );
                break;

            default:
                if ( INVALID_SET_FILE_POINTER == SetFilePointer( h_file, dw_chunk_data_size, nullptr, FILE_CURRENT ) ) return HRESULT_FROM_WIN32( GetLastError( ) );
            }

            dw_offset += sizeof( DWORD ) * 2;

            if ( dw_chunk_type == fourcc ) {
                dw_chunk_size          = dw_chunk_data_size;
                dw_chunk_data_position = dw_offset;
                return S_OK;
            }

            dw_offset += dw_chunk_data_size;

            if ( bytes_read >= dw_riff_data_size ) return S_FALSE;
        }

        return S_OK;
    }

    HRESULT read_chunk_data( HANDLE hFile, void* buffer, DWORD buffer_size, const DWORD buffer_offset ){
        HRESULT hr = S_OK;
        if ( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, buffer_offset, nullptr, FILE_BEGIN ) ) return HRESULT_FROM_WIN32( GetLastError( ) );
        DWORD dwRead;
        if ( 0 == ReadFile( hFile, buffer, buffer_size, &dwRead, nullptr ) ) hr = HRESULT_FROM_WIN32( GetLastError( ) );
        return hr;
    }

    auto play( std::string_view path, float volume ) -> bool{
        if ( !m_xaudio2 ) { initialize( ); }

        WAVEFORMATEXTENSIBLE wfx           = { 0 };
        XAUDIO2_BUFFER       buffer        = { 0 };
        TCHAR*               str_file_name = new TCHAR[ path.size( ) + 1 ];

        str_file_name[ path.size( ) ] = 0;
        std::ranges::copy( path, str_file_name );

        // Open the file
        const HANDLE h_file = CreateFile(
            str_file_name,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if ( INVALID_HANDLE_VALUE == h_file ) return HRESULT_FROM_WIN32( GetLastError( ) );

        if ( INVALID_SET_FILE_POINTER == SetFilePointer( h_file, 0, nullptr, FILE_BEGIN ) ) return HRESULT_FROM_WIN32( GetLastError( ) );
        DWORD dw_chunk_size;
        DWORD dw_chunk_position;
        //check the file type, should be fourccWAVE or 'XWMA'
        find_chunk( h_file, fourccRIFF, dw_chunk_size, dw_chunk_position );
        DWORD filetype;
        read_chunk_data( h_file, &filetype, sizeof( DWORD ), dw_chunk_position );
        if ( filetype != fourccWAVE ) return S_FALSE;

        find_chunk( h_file, fourccFMT, dw_chunk_size, dw_chunk_position );
        read_chunk_data( h_file, &wfx, dw_chunk_size, dw_chunk_position );
        find_chunk( h_file, fourccDATA, dw_chunk_size, dw_chunk_position );
        BYTE* pDataBuffer = new BYTE[ dw_chunk_size ];
        read_chunk_data( h_file, pDataBuffer, dw_chunk_size, dw_chunk_position );

        buffer.AudioBytes = dw_chunk_size;         // size of the audio buffer in bytes
        buffer.pAudioData = pDataBuffer;           // buffer containing audio data
        buffer.Flags      = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
        IXAudio2SourceVoice* pSourceVoice;
        HRESULT              result;
        if ( FAILED( result = m_xaudio2->CreateSourceVoice(&pSourceVoice, reinterpret_cast< WAVEFORMATEX* >( &wfx )) ) ) return false;
        if ( FAILED( result = pSourceVoice->SubmitSourceBuffer(&buffer) ) ) return false;
        pSourceVoice->SetVolume( volume );

        if ( FAILED( result = pSourceVoice->Start(0) ) ) return false;

        return true;
    }
}
