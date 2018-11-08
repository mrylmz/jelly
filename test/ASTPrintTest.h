//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <gtest/gtest.h>
#include <AST/AST.h>
#include <Parse/Parse.h>
#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <fstream>

#define AST_PRINT_TEST(__DIR__) \
INSTANTIATE_TEST_CASE_P(__DIR__, ASTPrintTest, testing::ValuesIn(read_test_files_from_subdirectory(#__DIR__)))

struct ASTPrintTestParameter {
    std::string directory;
    std::string source_filename;
    std::string target_filename;
};

class ASTPrintTest : public testing::TestWithParam<ASTPrintTestParameter> {};

std::string get_source_file_path();

bool read_file_content(std::string filepath, std::string& content);

bool write_file_content(std::string filepath, std::string content);

void parse_file_content(ASTContext& context, std::string filepath);

std::string print_ast(const ASTContext& context);

bool record_ast(std::string directory, std::string filename);

std::vector<ASTPrintTestParameter> read_test_files_from_subdirectory(std::string subdirectory);
