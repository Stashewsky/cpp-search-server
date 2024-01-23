#pragma once
#include "document.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <map>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 10e-6;

class SearchServer {
public:
    template <typename Collection>
    explicit SearchServer(const Collection& c);

    explicit SearchServer(const std::string& s);

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& user_ratings);

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const;

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const;
    vector<Document> FindTopDocuments(const string& raw_query) const;
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;
    int GetDocumentCount();
    int GetDocumentId(int index) const;

private:

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };

    struct DocumentRatingStatus{
        DocumentStatus status;
        int rating;
    };

    vector<int> document_count_;
    map<string, map<int, double>> documents_index_tf_;
    set<string> stop_words_;
    map<int, DocumentRatingStatus> all_documents_;

    static bool IsValidWord(const string& word);
    static int ComputeAverageRating(const vector<int>& ratings);
    vector<string> SplitIntoWordsNoStop(const string& text) const;
    void ParseQueryWord(const string& s, Query& result) const;
    Query ParseQuery(const string& text) const;
    double GetIDF(const string& word) const;

    template<typename Predicat>
    vector<Document> FindAllDocuments(const Query& query_words, Predicat predicat) const;
};

template <typename Collection>
SearchServer::SearchServer(const Collection& c){
    for (auto& word : c) {

        if(!IsValidWord(word)){
            throw invalid_argument("Ошибка! Стоп-слова содержат спецсимволы!"s);
        }
        if(!word.empty()){
            stop_words_.insert(word);
        }
    }
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
    auto query_words = ParseQuery(raw_query);
    vector<Document> result = FindAllDocuments(query_words, document_predicate);
    sort(result.begin(), result.end(),[](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON){
            return lhs.rating > rhs.rating;
        }
        return lhs.relevance > rhs.relevance;
    });
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;
}

template<typename Predicat>
vector<Document> SearchServer::FindAllDocuments(const Query& query_words, Predicat predicat) const {
    vector<Document> matched_documents;
    map<int, double> document_id_relevance;

    for (const string& word : query_words.plus_words) {
        if(documents_index_tf_.count(word)){
            for(const auto& [id, tf] : documents_index_tf_.at(word)){
                auto current_document_id = all_documents_.at(id);
                if(predicat(id, current_document_id.status, current_document_id.rating)){
                    double idf = GetIDF(word);
                    document_id_relevance[id] += tf * idf;
                }
            }
        }
    }

    for(const string& word : query_words.minus_words){
        if (documents_index_tf_.count(word)){
            for(const auto& [id, tf] : documents_index_tf_.at(word)){
                document_id_relevance.erase(id);
            }
        }
    }

    for(const auto& [id, relevance] : document_id_relevance){
        matched_documents.push_back({id, relevance, all_documents_.at(id).rating});
    }

    return matched_documents;
}