#pragma once
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int time = 0;
        int results_num = 0;
    };

    const SearchServer& server_;
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int current_time_;
    int empty_requests_;

    void AddRequest(int results);
};

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    vector<Document> result;
    result = server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(result.size());
    return result;
}
