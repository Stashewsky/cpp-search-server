#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum DocumentStatus{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id;
    double relevance;
    int rating;
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    
    return words;
}

class SearchServer {
    
public:

    void SetStopWords(const string& text) {
        
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document,DocumentStatus status, const vector<int>& user_ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double term_frequency = 1.0/words.size();
        for(const string& word : words){
            documents_index_tf_[word][document_id]+=term_frequency;
            }
        all_documents_[document_id] = {status, ComputeAverageRating(user_ratings)};
        document_count_++;
        }

    template<typename Predicat>
    vector<Document> FindTopDocuments(const string& raw_query, Predicat predicat)const {
        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words, predicat);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON){
                    return lhs.rating > rhs.rating;
                }else{
                    return lhs.relevance > rhs.relevance;
                }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments(raw_query, [&document_status](int document_id, const DocumentStatus& status, int rating){return status == document_status;});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(string raw_query, const int document_id) const {
        Query query = ParseQuery(raw_query);
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

private:

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };

    struct DocumentRatingStatus{
        DocumentStatus status;
        int rating;
    };

    int document_count_ = 0;
    map<string, map<int, double>> documents_index_tf_;
    set<string> stop_words_;
    map<int, DocumentRatingStatus> all_documents_;

    static int ComputeAverageRating(const vector<int>& ratings) {
        int sum = accumulate(ratings.begin(), ratings.end(), 0); //сначала хотел записать отдной строчкой в return, но подумал, что так будет лучше
        return sum / static_cast<int>(ratings.size());
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
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
    Query ParseQuery(const string& text) const {
        Query plus_minus_words;
        for(const string& s : SplitIntoWordsNoStop(text)){
            if(s[0] == '-' && s.size() > 1){
                const string k = s.substr(1);
                plus_minus_words.minus_words.insert(k);
            }else{
                plus_minus_words.plus_words.insert(s);
            }
        }
        return plus_minus_words;
    }

    double GetIDF(const string& word) const {
            double res = log(static_cast<double>(document_count_)/documents_index_tf_.at(word).size());
        return res;
        }

    template<typename Predicat>
    vector<Document> FindAllDocuments(const Query query_words, Predicat predicat) const {
        vector<Document> matched_documents;
        map<int, double> document_id_relevance;
        for (const string& word : query_words.plus_words) {
            if(documents_index_tf_.count(word)){
                for(const auto& [id, tf] : documents_index_tf_.at(word)){
                    if(predicat(id, all_documents_.at(id).status, all_documents_.at(id).rating)){
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

};

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}
int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}