//
// Created by Станислав on 22.01.2024.
//

#include "search_server.h"
#include <numeric>
#include "string_processing.h"
SearchServer::SearchServer(const std::string &s): SearchServer(SplitIntoWords(s)){}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& user_ratings) {
    if(document_id < 0){
        throw invalid_argument("Ошибка! Документ, который вы пытаетесь добавить имеет отрицательный id!"s);
    }else if(all_documents_.count(document_id)){
        throw invalid_argument("Ошибка! Документ с таким id уже существует!"s);
    }else if(!IsValidWord(document)){
        throw invalid_argument("Ошибка! Текст документа содержит спецсимволы!"s);
    }else{
        const vector<string> words = SplitIntoWordsNoStop(document);
        double term_frequency = 1.0/words.size();

        for(const string& word : words){
            documents_index_tf_[word][document_id]+=term_frequency;
        }

        all_documents_[document_id] = {status, ComputeAverageRating(user_ratings)};
        document_count_.push_back(document_id);
    }
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
    return FindTopDocuments(raw_query, [&document_status](int document_id, const DocumentStatus& status, int rating){
        return status == document_status;});
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    auto query = ParseQuery(raw_query);
    vector<string> matched_words;
    DocumentStatus status = all_documents_.at(document_id).status;
    for(const string& word : query.minus_words){
        if(documents_index_tf_.at(word).count(document_id)){
            return tuple(matched_words, status);
        }
    }
    for(const string& word : query.plus_words){
        if(documents_index_tf_.at(word).count(document_id)){
            matched_words.push_back(word);
        }
    }

    return tuple(matched_words, status);
}

int SearchServer::GetDocumentCount(){
    return all_documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    return document_count_.at(index);
}

bool SearchServer::IsValidWord(const string& word) {
    //проверка на содержание спецсимволов
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    int sum = accumulate(ratings.begin(), ratings.end(), 0);
    return sum / static_cast<int>(ratings.size());
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    if(stop_words_.empty()){
        return SplitIntoWords(text);
    }else{
        for (const string& word : SplitIntoWords(text)) {
            if (stop_words_.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }
}

void SearchServer::ParseQueryWord(const string& s, Query& result) const {
    if(s[0] == '-' && s[1] == '-'){
        throw invalid_argument("Ошибка! В слове запроса содержится два минуса подряд (--)!"s);
    }else if(s[0] == '-' && s.size() == 1){
        throw invalid_argument("Ошибка! В слове запроса содержится (-) минус без слова!"s);
    }else if(s[0] == '-'){
        const string k = s.substr(1);
        result.minus_words.insert(k);
    }else{
        result.plus_words.insert(s);
    }
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    if(text.empty()){
        throw invalid_argument("Ошибка! Вы передаете пустой запрос!"s);
    }else if(!IsValidWord(text)){
        throw invalid_argument("Ошибка! Запрос содержит спецсимволы!");
    }

    Query result;
    for(const string& s : SplitIntoWordsNoStop(text)){
        ParseQueryWord(s, result);
    }
    return result;
}

double SearchServer::GetIDF(const string& word) const {
    double res = log(static_cast<double>(all_documents_.size())/documents_index_tf_.at(word).size());
    return res;
}