#pragma once
#define _CRT_SECURE_NO_WARNINGS       // fopen vs fopen_s

#include <core/platform/platform.hpp> // A_NODISCARD
#include <core/types/types.hpp>       // byte, byte_vec
#include <core/common/common.hpp>     // assert_msg

#include <string_view>                // std::string_view
#include <memory>                     // std::unique_ptr
#include <cstdio>					  // fopen, fclose, fseek, ftell, fread, fwrite


namespace args::core::filesystem {
    /**@brief Open File in binary mode and write to buffer
     *
     * @param [in] path the path of the file to open
     * @return a vector of bytes with the contents of the file at path
     */
    A_NODISCARD inline  byte_vec read_file(std::string_view path)
    {

        //create managed FILE ptr
        const std::unique_ptr<FILE,decltype(&fclose)> file(
            fopen(std::string(path).c_str(),"r+b"),
            fclose // deleter is fclose
        );
        
        assert_msg("could not open file",file);

        //get size and create container of that size
        fseek(file.get(),0L,SEEK_END);
        byte_vec container(ftell(file.get()));
        fseek(file.get(),0L,SEEK_SET);
        
        //read file 
        fread(container.data(),sizeof(byte_t),container.size(),file.get());

        return container;
    }

    /**@brief Open file in binary mode to write the buffer to it
     *
     * @param [in] path the path of the file you want to write to
     * @param [in] container the buffer you want to write to the file
     */
    inline void write_file(std::string_view path,const byte_vec& container)
    {

        //create managed FILE ptr
        const std::unique_ptr<FILE,decltype(&fclose)> file(
            fopen(std::string(path).c_str(),"wb"),
            fclose
        );

        assert_msg("could not open file",file);

        // read data
        fwrite(container.data(),sizeof(byte_t),container.size(),file.get());

    }


    /**@brief work the same as the above, but the path
     *  parameter is replaced by the string literal
     */
    namespace literals
    {
        byte_vec operator""_readfile(const char * str, std::size_t len)
        {
            return ::args::core::filesystem::read_file(std::string_view(str,len));
        }

        auto operator""_writefile(const char * str,std::size_t len)
        {
            return [path=std::string_view(str,len)](const byte_vec& container)
            {
                write_file(path,container);
            };
        }
    }
}

#undef _CRT_SECURE_NO_WARNINGS

 //code example
/*
void example()
{
    using namespace  args::core::fs::literals;

    auto file1 = "hello_world.cpp"_readfile;
    auto file2 = args::core::fs::read_file("hello_world.cpp"); 

    "hello_world2.cpp"_writefile(file2);
    args::core::fs::write_file("hello_world2.cpp",file1);
}
*/