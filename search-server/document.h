#pragma once
#include <ostream>
#include <string>
#include <vector>
struct Document {
    Document() noexcept;
    Document(int id, double relevance, int rating);

    int id = 0;
    double relevance = 0.0;
    int rating = 0.0;
};
enum DocumentStatus{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

void PrintMatchDocumentResult(std::tuple<std::vector<std::string>, DocumentStatus> s);
void PrintDocument(const Document& document);

std::ostream& operator <<(std::ostream& out, const Document& document);
