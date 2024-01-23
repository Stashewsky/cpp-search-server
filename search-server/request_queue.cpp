#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
        : server_(search_server),
          current_time_(0),
          empty_requests_(0)
{
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    vector<Document> result;
    result = server_.FindTopDocuments(raw_query, status);
    AddRequest(result.size());
    return result;
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    vector<Document> result;
    result = server_.FindTopDocuments(raw_query);
    AddRequest(result.size());
    return result;
}
int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return empty_requests_;
}
void RequestQueue::AddRequest(int results){
    current_time_++;
    QueryResult request = {current_time_,results};
    if(!requests_.empty() && current_time_ - requests_.front().time >= min_in_day_){
        if(requests_.front().results_num == 0){
            empty_requests_--;
        }
        requests_.pop_front();
    }
    requests_.push_back(request);
    if(request.results_num == 0){
        empty_requests_++;
    }
}