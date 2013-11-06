#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <sys/stat.h>
#include <Windows.h>
#include <math.h>

using namespace std;

FILE* gErrorLog = 0;
char gErrorLogPath[1024];

void begin_log( char const* input_path )
{
    sprintf( gErrorLogPath, "%s.txt", input_path );
    gErrorLog = 0;
}

void end_log()
{
    if( gErrorLog )
    {
        fclose( gErrorLog );
    }
    
}

void error( char const* format, ... )
{
    if( 0 == gErrorLog )
    {
        gErrorLog = fopen( gErrorLogPath, "w" );
        fprintf( gErrorLog, "Building '%s'.\n", gErrorLogPath );
    }

	char message[4096];
	va_list argptr;
	va_start( argptr, format );
	vsprintf( message, format, argptr );
	va_end( argptr );
	OutputDebugString( message );
	printf( message );

    fprintf( gErrorLog, message );
    fflush( gErrorLog );
    fclose( gErrorLog );
    
    system( gErrorLogPath );

	exit( -1 );
}

time_t get_last_modified( char const* path )
{
    struct stat info;
    int result = stat( path, &info );
    if( result == 0 )
    {
        return info.st_mtime;
    }
    else
    {
        return 0;
    }
    
}

bool is_more_recent( char const* path_a, char const* path_b )
{
    time_t a_time = get_last_modified( path_a );
    time_t b_time = get_last_modified( path_b );
    return a_time < b_time;
}

bool exists( char const* path )
{
	struct _stat info;
	int result = _stat( path, &info );
	return result == 0;
}

char const* get_file_name( char const* path )
{
	char const* name = strrchr( path, '/' );
	if( !name )
	{
		return path;		
	}
	return name;
}

void get_output_file_path( char const* input_file_path, char* output_file_path )
{
    strcpy( output_file_path, input_file_path );
    strcpy( strrchr( output_file_path, '.' ), ".xml" );
}

void get_image_name( char const* path, char* name )
{
    char const* name_with_ext = strrchr( path, '/' );
    if( !name_with_ext )
    {
        name_with_ext = path;		
    }
    else
    {
        ++name_with_ext;
    }

    char const* extension = strrchr( name_with_ext, '.' );
    int length = extension - name_with_ext;
    memcpy( name, name_with_ext, length );
    name[length] = 0;
}

char const* skip_slash( char const* name )
{
    if( name[0] == '/' )
    {
        ++name;
    }
    return name;
}


void get_folder( char const* path, char* folder )
{
    int length = strrchr( path, '\\' ) - path + 1;
    memcpy( folder, path, length );
    folder[length] = 0;
}

char appplication_folder[1024];
void set_application_folder( char const* application_path )
{
    get_folder( application_path, appplication_folder );
}

char const* get_application_folder()
{
    return appplication_folder;
}

BOOL run( char* command_line )
{
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    BOOL result = CreateProcess( 0, command_line, 0, 0, FALSE, 0, NULL, NULL, &si, &pi );

    if( !result )
    {
        printf( "ERROR: Could not run '%s'!\n", command_line );
        return -1;
    }
    else
    {
        WaitForSingleObject( pi.hProcess, INFINITE );
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }
}

int main( int argument_count, char** arguments )
{
    set_application_folder( arguments[0] );
    begin_log( arguments[1] );

	if( argument_count != 3 )
	{
		error( "ERROR: Invalid number of arguments!\n" );
	}

	char const* input_file_path = arguments[1];

	if( !exists( input_file_path ) )
	{
		error( "ERROR: Could not open '%s'!\n", input_file_path );
	}

    char input_folder[1024];
    get_folder( input_file_path, input_folder );

    char output_package_file_path[MAX_PATH];
    get_output_file_path( input_file_path, output_package_file_path );

	char build_tool_path[1024];
	sprintf(build_tool_path, "%scompiler_scripts/image_build.py", get_application_folder());

    if( is_more_recent( input_file_path, output_package_file_path ) && is_more_recent( arguments[0], output_package_file_path ) && is_more_recent( build_tool_path, output_package_file_path  ) )
    {
        return 0;
    }

    std::vector< char const* > image_paths;

	char command_line[4096];

    char const* output_dir = "data";
    if( strstr( input_folder, "mods" ) )
    {
        output_dir = ".";
    }

    sprintf( command_line, "%s..\\buildtools\\windows\\Python27\\python.exe \"%s\" \"%s\"", get_application_folder(), build_tool_path, input_file_path);
    int result = run( command_line );
    if( 0 != result )
    {
        error( "ERROR: Could not run '%s'!\n", command_line );
    }

    end_log();

	return 0;
}