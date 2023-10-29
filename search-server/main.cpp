#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    double relevance;
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

    void SetDocumentCount(const int& count){
        document_count_ = count;
    }
    
   double GetTermFrequency(const vector<string>& words, const string& word, const double& words_size){
       return static_cast<double>(count(words.begin(), words.end(), word)) / words_size;
   }
    
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double words_size = words.size();
        for(const string word : words){
            double term_frequency = GetTermFrequency(words, word, words_size);
            documents_index_tf_[word].insert({document_id, term_frequency});
            }
        }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };
    int document_count_ = 0;
    map<string, map<int, double>> documents_index_tf_;
    set<string> stop_words_;

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

    vector<Document> FindAllDocuments(const Query query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_id_relevance;
        for (const string& word : query_words.plus_words) {
            if(documents_index_tf_.count(word)){
                for(const auto& [id, tf] : documents_index_tf_.at(word)){
                    double idf = GetIDF(word);
                    document_id_relevance[id] += tf * idf;
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
                matched_documents.push_back({id, relevance});
            }
        return matched_documents;
    }
};
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    search_server.SetDocumentCount(document_count);
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}