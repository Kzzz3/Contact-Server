#ifndef COREALGORITHM_H
#define COREALGORITHM_H

#include <set>
#include <unordered_map>

#include "../Server/Server.h"
#include "../cppjieba/Jieba.hpp"
using namespace std;
using namespace cppjieba;
const char *const DICT_PATH = "../dict/jieba.dict.utf8";
const char *const HMM_PATH = "../dict/hmm_model.utf8";
const char *const USER_DICT_PATH = "../dict/user.dict.utf8";
const char *const IDF_PATH = "../dict/idf.utf8";
const char *const STOP_WORD_PATH = "../dict/stop_words.utf8";

class Server;
class MatchScheduler
{
public:
    static constexpr int MATCH_INTERVAL = 100;
    static constexpr double MATCH_SIMILARITY = 0.5;

    Server *m_pServ;
    std::thread m_thdWorker;
    cppjieba::Jieba m_jbDivider;
    boost::asio::io_context m_ioContext;
    boost::asio::deadline_timer m_timerClock;
    std::map<std::shared_ptr<Client>, std::shared_ptr<Packet>> m_ClientPackets;
    std::map<std::shared_ptr<Client>, unordered_map<string, int>> m_ClientWords;

public:
    MatchScheduler() = delete;
    MatchScheduler(Server *_serv);
    ~MatchScheduler();

    void Matching();
    void RemoveMatchClient(std::shared_ptr<Client> clientDst);
    void AddMatchClient(std::shared_ptr<Client> clientDst,
                        std::shared_ptr<Packet> pktMatch);

    unordered_map<string, int> GetWordCount(const string &text);
    double GetVectorLength(const unordered_map<string, int> &word_count);
    double CalculateSimilarity(const unordered_map<string, int> &word_count1,
                               const unordered_map<string, int> &word_count2);
};
#endif // COREALGORITHM_H
