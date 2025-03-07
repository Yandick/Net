#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <cmath>
#include <algorithm>
#include <map>
#include <iomanip>
#include <unordered_set>

// 节点结构体
struct Node
{
    int id;
    double x, y;
    std::vector<int> neighbors;  // 邻居节点
    std::deque<int> cache;       // 缓存队列
    int next_expected_block = 1; // 下一个期望的数据块
};

// 网络连接结构体
struct Connection
{
    int source, target;
    double bandwidth; // 传输速率 (KB/s)
};

// 事件结构体
struct Event
{
    double time; // 事件发生时间
    int type;    // 事件类型 (0: 生成数据块, 1: 传输数据块, 2: 请求数据块)
    int source, target;
    int block_id; // 数据块序号

    // 优先队列比较函数
    bool operator>(const Event &other) const
    {
        return time > other.time;
    }
};

// 全局变量
std::vector<Node> nodes;                                                        // 所有节点
std::vector<Connection> connections;                                            // 所有连接
std::priority_queue<Event, std::vector<Event>, std::greater<Event>> eventQueue; // 事件队列
int currentBlockId = 1;                                                         // 当前数据块序号
const int blockSize = 1;                                                        // 数据块大小 (KB)
const int serverId = 0;                                                         // 服务器 ID
double totalTime = 10;                                                          // 总模拟时间 (秒)
const int M = 5;                                                                // 连续播放所需的数据块数
const int cacheSize = 50;                                                       // 客户端缓存大小
const double distancemaxm = 1000.00 * sqrt(2);

// 初始化网络
void initializeNetwork(int N, int t)
{
    // 生成服务器和客户端节点
    for (int i = 0; i <= N; ++i)
    {
        Node node;
        node.id = i;
        node.x = rand() % 1000;
        node.y = rand() % 1000;
        nodes.push_back(node);
    }

    // 随机建立双向连接
    std::map<std::pair<int, int>, bool> connectionMap; // 避免重复连接
    double minBandwidth = 100.0, maxBandwidth = 0.0;
    for (int i = 0; i <= N; ++i)
    {
        std::unordered_set<int> selectedNeighbors; // 已选择的邻居
        while (selectedNeighbors.size() < t)
        {
            int neighbor = rand() % (N + 1);
            if (neighbor != i && selectedNeighbors.find(neighbor) == selectedNeighbors.end())
            {
                selectedNeighbors.insert(neighbor);

                // 添加双向连接
                nodes[i].neighbors.push_back(neighbor);
                nodes[neighbor].neighbors.push_back(i);

                // 计算传输速率
                double distance = sqrt(pow(nodes[i].x - nodes[neighbor].x, 2) + pow(nodes[i].y - nodes[neighbor].y, 2));
                double bandwidth = (100.0 / distancemaxm) * distance;   // 传输速率与距离成反比
                bandwidth = std::min(std::max(bandwidth, 20.0), 100.0); // 限制在 [20KB/s, 100KB/s]

                // 更新最小和最大传输速率
                minBandwidth = std::min(minBandwidth, bandwidth);
                maxBandwidth = std::max(maxBandwidth, bandwidth);

                // 添加连接
                connections.push_back({i, neighbor, bandwidth});
                connections.push_back({neighbor, i, bandwidth});
            }
        }
    }

    // 打印调试信息
    std::cout << "Bandwidth range: " << minBandwidth << " KB/s to " << maxBandwidth << " KB/s" << std::endl;
    std::cout << "Cache size: " << cacheSize << std::endl;
    std::cout << "Number of neighbors per node: " << t << std::endl;
}

// 调度事件
void scheduleEvent(double time, int type, int source, int target, int block_id)
{
    eventQueue.push({time, type, source, target, block_id});
}

// 处理生成数据块事件
void handleBlockGeneration(double &currentTime)
{
    // 服务器每秒生成 30 个数据块
    for (int i = 0; i < 30; ++i)
    {
        nodes[serverId].cache.push_back(currentBlockId);
        if (nodes[serverId].cache.size() > cacheSize)
        {
            nodes[serverId].cache.pop_front();
        }

        // 将数据块发送给邻居
        for (int neighbor : nodes[serverId].neighbors)
        {
            double bandwidth;
            double distance = sqrt(pow(nodes[serverId].x - nodes[neighbor].x, 2) + pow(nodes[serverId].y - nodes[neighbor].y, 2));
            bandwidth = (100.0 / distancemaxm) * distance;
            bandwidth = std::min(std::max(bandwidth, 20.0), 100.0);
            double transmissionTime = blockSize / bandwidth;
            scheduleEvent(currentTime + transmissionTime, 1, serverId, neighbor, currentBlockId);
        }

        currentBlockId++;
    }

    // 调度下一个生成数据块事件
    scheduleEvent(currentTime + 1, 0, serverId, -1, -1);
}

// 处理传输数据块事件
void handleBlockTransmission(double &currentTime, int source, int target, int block_id)
{
    // 将数据块加入目标节点缓存
    nodes[target].cache.push_back(block_id);
    if (nodes[target].cache.size() > cacheSize)
    {
        nodes[target].cache.pop_front();
    }

    // 更新 next_expected_block
    if (block_id == nodes[target].next_expected_block)
    {
        nodes[target].next_expected_block++;
    }

    // 打印调试信息
    std::cout << "Node " << target << " received block " << block_id << ", next_expected_block=" << nodes[target].next_expected_block << std::endl;

    // 目标节点向邻居请求缺失的数据块
    // for (int neighbor : nodes[target].neighbors)
    // {
    //     if (neighbor != source)
    //     {
    //         scheduleEvent(currentTime, 2, target, neighbor, block_id);
    //     }
    // }
}

// 处理请求数据块事件
void handleBlockRequest(double &currentTime, int source, int target, int block_id)
{
    // 检查目标节点是否有请求的数据块
    if (std::find(nodes[target].cache.begin(), nodes[target].cache.end(), block_id) != nodes[target].cache.end())
    {
        double distance = sqrt(pow(nodes[source].x - nodes[target].x, 2) + pow(nodes[source].y - nodes[target].y, 2));
        double bandwidth = (100.0 / distancemaxm) * distance;
        bandwidth = std::min(std::max(bandwidth, 20.0), 100.0);
        double transmissionTime = blockSize / bandwidth;
        scheduleEvent(currentTime + transmissionTime, 1, target, source, block_id);
    }
    return;
}

// 模拟运行
void simulate()
{
    double currentTime = 0.000;
    while (!eventQueue.empty() && currentTime < totalTime)
    {
        Event e = eventQueue.top();
        eventQueue.pop();
        currentTime = e.time;

        // 打印调试信息
        std::cout << "Processing event: time=" << e.time << ", type=" << e.type << ", source=" << e.source << ", target=" << e.target << ", block_id=" << e.block_id << std::endl;

        switch (e.type)
        {
        case 0: // 生成数据块
            handleBlockGeneration(currentTime);
            break;
        case 1: // 传输数据块
            handleBlockTransmission(currentTime, e.source, e.target, e.block_id);
            break;
        case 2: // 请求数据块
            handleBlockRequest(currentTime, e.source, e.target, e.block_id);
            break;
        }

        // totalTime--;
    }
}

// 计算流畅度
double calculateSmoothness(const Node &client)
{
    int continuousBlocks = 0;
    int nextExpected = client.next_expected_block;
    for (int block : client.cache)
    {
        if (block == nextExpected)
        {
            continuousBlocks++;
            nextExpected++;
        }
        else
        {
            continuousBlocks = 0;
        }
    }
    return (continuousBlocks >= M) ? 1.0 : 0.0;
}

// 主函数
int main()
{
    int N = 100; // 客户端数量
    int t = 10;  // 每个节点的邻居数

    // 初始化网络
    initializeNetwork(N, t);

    // 启动模拟
    scheduleEvent(0.0, 0, serverId, -1, -1); // 服务器开始生成数据块
    simulate();

    // 计算每个客户端的流畅度
    double totalSmoothness = 0.0;
    for (int i = 1; i <= N; ++i)
    {
        double smoothness = calculateSmoothness(nodes[i]);
        totalSmoothness += smoothness;
        std::cout << "Client " << i << " smoothness: " << std::fixed << std::setprecision(2) << smoothness * 100 << "%" << std::endl;
    }

    std::cout << "Average smoothness: " << std::fixed << std::setprecision(2) << (totalSmoothness / N) * 100 << "%" << std::endl;

    return 0;
}