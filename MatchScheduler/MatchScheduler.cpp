#include "MatchScheduler.h"

MatchScheduler::MatchScheduler(Server *_serv)
    : m_pServ(_serv),
      m_timerClock(m_ioContext,
                   boost::posix_time::milliseconds(MATCH_INTERVAL)),
      m_jbDivider(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH,
                  STOP_WORD_PATH)
{
    std::thread thdTemp([this]()
                        {
    m_ioContext.post([this]() { Matching(); });
    m_ioContext.run(); });
    m_thdWorker = std::move(thdTemp);
}
MatchScheduler::~MatchScheduler() {}
void MatchScheduler::Matching()
{
    int dSimilarity = 0;
    std::shared_ptr<Client> clientMatch = nullptr;
    for (auto &[client, words] : m_ClientWords)
    {
        if (m_ClientWords.size() < 2)
            break;
        for (auto &[client2, words2] : m_ClientWords)
        {
            if (client == client2)
                continue;

            double dSimilarityTemp = CalculateSimilarity(words, words2);
            if (dSimilarityTemp >= dSimilarity)
            {
                dSimilarity = dSimilarityTemp;
                clientMatch = client2;
            }
            if (dSimilarity > MATCH_SIMILARITY)
                break;
        }
        if (clientMatch != nullptr)
        {
            m_ClientWords.erase(client);
            m_ClientWords.erase(clientMatch);

            std::cout << "similarity:" << dSimilarity << std::endl;
            // 发送匹配成功的消息
            m_pServ->_io_context.post(
                std::bind(Server::match_success, client,m_ClientPackets[client],clientMatch, m_ClientPackets[clientMatch], m_pServ));
            break;
        }
    }
    m_ioContext.post([this]()
                     { Matching(); });
    m_timerClock.wait();
}
void MatchScheduler::AddMatchClient(std::shared_ptr<Client> clientDst,
                                    std::shared_ptr<Packet> pktMatch)
{
    m_ioContext.post([this, clientDst, pktMatch]()
                     {
    unordered_map<string, int> words = GetWordCount(pktMatch->get_pbody());
    m_ClientWords[clientDst] = words; 
    m_ClientPackets[clientDst] = pktMatch; });
}
void MatchScheduler::RemoveMatchClient(std::shared_ptr<Client> clientDst)
{
    m_ioContext.post([this, clientDst]()
                     {
    if (m_ClientWords.find(clientDst) != m_ClientWords.end())
      m_ClientWords.erase(clientDst); 
    if (m_ClientPackets.find(clientDst) != m_ClientPackets.end())
      m_ClientPackets.erase(clientDst); });
}
unordered_map<string, int> MatchScheduler::GetWordCount(const string &text)
{
    vector<string> words;
    unordered_map<string, int> word_count;

    m_jbDivider.Cut(text, words, true);
    for (const auto &word : words)
    {
        if (word_count.find(word) == word_count.end())
        {
            word_count[word] = 1;
        }
        else
        {
            word_count[word]++;
        }
    }
    return word_count;
}
double MatchScheduler::GetVectorLength(
    const unordered_map<string, int> &word_count)
{
    double sum = 0;
    for (const auto &kv : word_count)
    {
        sum += kv.second * kv.second;
    }
    return sqrt(sum);
}
double MatchScheduler::CalculateSimilarity(
    const unordered_map<string, int> &word_count1,
    const unordered_map<string, int> &word_count2)
{
    double dot_product = 0;
    for (const auto &kv : word_count1)
    {
        if (word_count2.find(kv.first) != word_count2.end())
        {
            dot_product += kv.second * word_count2.at(kv.first);
        }
    }
    double length1 = GetVectorLength(word_count1);
    double length2 = GetVectorLength(word_count2);
    return dot_product / (length1 * length2);
}
