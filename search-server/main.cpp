#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
    Document() = default;

    Document(int a, double b, int c){
        id = a;
        relevance = b;
        rating = c;
    }

    int id = 0;
    double relevance = 0;
    int rating = 0;
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
    template <typename Collection>
    explicit SearchServer(const Collection& c){
        for (auto& word : c) {

            if(!IsValidWord(word)){
                throw invalid_argument("Ошибка! Стоп-слова содержат спецсимволы!"s);
            }else if(!word.empty()){
                stop_words_.insert(word);
            }
        }
    }

    explicit SearchServer(const string& s){
        if(!IsValidWord(s)){
            throw invalid_argument("Ошибка! Стоп-слова содержат спецсимволы!"s);
        }

        for(string& word : SplitIntoWords(s)){
            stop_words_.insert(word);
        }
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& user_ratings) {
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

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        auto query_words = ParseQuery(raw_query);
        vector<Document> result = FindAllDocuments(query_words, document_predicate);

            sort(result.begin(), result.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (abs(lhs.relevance - rhs.relevance) < EPSILON){
                        return lhs.rating > rhs.rating;
                    }else{
                        return lhs.relevance > rhs.relevance;
                    }
                });
            if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
                result.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
        return result;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments(raw_query, [&document_status](int document_id, const DocumentStatus& status, int rating){
                                                            return status == document_status;});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
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

    int GetDocumentCount(){
        return all_documents_.size();
    }

    int GetDocumentId(int index) const {
        if(index < 0 || index > all_documents_.size()){
            throw out_of_range("Ошибка! Указанный индекс выходит за пределы допустимых значений!"s);
        }
        return document_count_[index];
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

    vector<int> document_count_;
    map<string, map<int, double>> documents_index_tf_;
    set<string> stop_words_;
    map<int, DocumentRatingStatus> all_documents_;

    static bool IsValidWord(const string& word) {
        //проверка на содержание спецсимволов
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int sum = accumulate(ratings.begin(), ratings.end(), 0);
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
        if(text.empty()){
            throw invalid_argument("Ошибка! Вы передаете пустой запрос!"s);
        }else if(!IsValidWord(text)){
            throw invalid_argument("Ошибка! Запрос содержит спецсимволы!");
        }

        Query result;
         for(const string& s : SplitIntoWordsNoStop(text)){
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
        return result;
    }

    double GetIDF(const string& word) const {
            double res = log(static_cast<double>(all_documents_.size())/documents_index_tf_.at(word).size());
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

void PrintMatchDocumentResult(tuple<vector<string>, DocumentStatus> s) {
    cout << "{ "s
         << "status = "s << static_cast<int>(get<1>(s)) << ", "s
         << "words ="s;
    for (const string& word : get<0>(s)) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("и в на"s);
    try
    {
        search_server.AddDocument(1, " asd"s, DocumentStatus::ACTUAL, {1,2,3});
        search_server.AddDocument(12, " asd"s, DocumentStatus::ACTUAL, {1,2,3});
        cout << search_server.GetDocumentId(4) << endl;

    }
    catch(const invalid_argument& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}