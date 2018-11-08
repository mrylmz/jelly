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

#include "ASTPrintTest.h"

std::string get_source_file_path() {
    std::string filepath = __FILE__;
    std::string directory = filepath.substr(0, filepath.rfind("/"));
    return directory;
}

bool read_file_content(std::string filepath, std::string& content) {
    std::fstream file;
    file.open(filepath, std::fstream::in);

    bool is_open = file.is_open();
    if (is_open) {
        content = {
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }

    file.close();
    return is_open;
}

bool write_file_content(std::string filepath, std::string content) {
    std::fstream file;
    file.open(filepath, std::fstream::out);

    bool is_open = file.is_open();
    if (is_open) {
        file.write(content.c_str(), content.length());
    }

    file.close();
    return is_open;
}

void parse_file_content(ASTContext& context, std::string filepath) {
    std::string content;

    if (!read_file_content(filepath, content)) {
        return;
    }

    Lexer    lexer(content.c_str());
    Parser   parser(context, lexer);

    parser.parse();
}

std::string print_ast(const ASTContext& context) {
    std::stringstream output;
    ASTPrinter        printer(output);

    printer.print(context);

    return output.str();
}

bool record_ast(std::string directory, std::string filename) {
    std::string working_directory = get_source_file_path().append("/").append(directory).append("/");
    std::string source_filepath = std::string(working_directory).append(filename);
    std::string target_filepath = std::string(source_filepath).append(".ast");

    ASTContext  context;
    parse_file_content(context, source_filepath);
    std::string source_ast = print_ast(context);
    std::string target_ast;

    return write_file_content(target_filepath, source_ast);
}

std::vector<ASTPrintTestParameter> read_test_files_from_subdirectory(std::string subdirectory) {
    std::string directory_path = get_source_file_path().append("/").append(subdirectory).append("/");
    DIR*        directory = opendir(directory_path.c_str());
    dirent*     directory_entry;

    std::vector<ASTPrintTestParameter> result;

    if (directory) {
        std::vector<std::string> sources;
        std::vector<std::string> targets;

        while ((directory_entry = readdir(directory)) != nullptr) {
            std::string name(directory_entry->d_name);

            if (name.rfind(".jelly.ast") != std::string::npos) {
                targets.push_back(name);
            } else if (name.rfind(".jelly") != std::string::npos) {
                sources.push_back(name);
            }
        }

        closedir(directory);

        for (auto it = sources.begin(); it != sources.end(); it++) {
            std::string name = std::string(*it).append(".ast");

            ASTPrintTestParameter parameter;
            parameter.directory = subdirectory;
            parameter.source_filename = *it;

            if (std::find(targets.begin(), targets.end(), name) != targets.end()) {
                parameter.target_filename = name;
            }

            result.push_back(parameter);
        }
    }

    return result;
}

//TEST_P(ASTPrintTest, parse_tree) {
//    auto parameter = GetParam();
//
//    std::string working_directory = get_source_file_path().append("/").append(parameter.directory).append("/");
//    std::string source_filepath = std::string(working_directory).append(parameter.source_filename);
//    std::string target_filepath = std::string(source_filepath).append(".ast");
//
//    EXPECT_FALSE(parameter.source_filename.empty());
//
//    if (parameter.target_filename.empty()) {
//        printf("[ REC      ] %s\n", parameter.source_filename.c_str());
//
//        if (!record_ast(parameter.directory, parameter.source_filename)) {
//            printf("[  FAILED  ] %s\n", parameter.source_filename.c_str());
//            FAIL();
//        } else {
//            printf("[       OK ] %s\n", parameter.source_filename.c_str());
//        }
//    }
//
//    ASTContext  context;
//    parse_file_content(context, source_filepath);
//    std::string source_ast = print_ast(context);
//    std::string target_ast;
//
//    printf("[ RUN      ] %s\n", parameter.source_filename.c_str());
//    if (read_file_content(target_filepath, target_ast)) {
//        EXPECT_STREQ(source_ast.c_str(), target_ast.c_str());
//    } else {
//        FAIL();
//    }
//}

TEST_P(ASTPrintTest, parse_tree) {
    auto parameter = GetParam();

    std::string working_directory = get_source_file_path().append("/").append(parameter.directory).append("/");
    std::string source_filepath = std::string(working_directory).append(parameter.source_filename);
    std::string target_filepath = std::string(source_filepath).append(".ast");

    EXPECT_FALSE(parameter.source_filename.empty());

    if (parameter.target_filename.empty()) {
        printf("[ REC      ] %s\n", parameter.source_filename.c_str());

        if (!record_ast(parameter.directory, parameter.source_filename)) {
            printf("[  FAILED  ] %s\n", parameter.source_filename.c_str());
            FAIL();
        } else {
            printf("[       OK ] %s\n", parameter.source_filename.c_str());
        }
    }

    ASTContext  context;
    parse_file_content(context, source_filepath);
    std::string source_ast = print_ast(context);
    std::string target_ast;

    printf("[ RUN      ] %s\n", parameter.source_filename.c_str());
    if (read_file_content(target_filepath, target_ast)) {
        EXPECT_STREQ(source_ast.c_str(), target_ast.c_str());
    } else {
        FAIL();
    }
}
