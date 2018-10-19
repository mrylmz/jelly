#include <benchmark/benchmark.h>
#include <Parse/Parse.h>

#warning remove BM_lexer_lex and add real use-cases

void BM_lexer_lex(benchmark::State& state) {
    size_t index = 0;

    for (auto _ : state) {
        Lexer lexer("func main() -> Void {}");
        Token token;

        while (!token.is(TOKEN_EOF)) {
            lexer.lex(token);
        }

        index++;
    }

    state.SetItemsProcessed(index);
}

void BM_parser_parse(benchmark::State& state) {
    size_t index = 0;

    for (auto _ : state) {
        Lexer  lexer("func main() -> Void {}");
        Parser parser(lexer);

        parser.parse();
        index++;
    }

    state.SetItemsProcessed(index);
}

BENCHMARK(BM_lexer_lex);
BENCHMARK(BM_parser_parse);
