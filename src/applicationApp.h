//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __Z_APPLICATIONAPP_H_
#define __Z_APPLICATIONAPP_H_

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <vector>
#include <numeric>
#include <chrono>
#include <set>

#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "veins/modules/utility/TimerManager.h"
#include <inet/common/ModuleAccess.h>
#include "veins_inet/VeinsInetMobility.h"

#include <math.h>

using namespace omnetpp;
using namespace inet;
using namespace std;


/**
 * TODO - Generated class
 */
class ApplicationApp : public cSimpleModule
{
    using TimePointType = std::chrono::time_point<std::chrono::high_resolution_clock>;
public:
    ApplicationApp(){}
    ~ApplicationApp(){}
public:
    UdpSocket socket;
    int localPort_;
    int size_;

    IMobility *mobility = nullptr;

    double period__;//表示发送间隔
    cMessage* sendPacket;

    double r;//表示发送间隔的更新频率
    double t_send;//暂存上一个数据包的发送时间
    double t_recv;//暂存上一个数据包的接收时间
    double aoi_last;//暂存上一轮的平均AOI
    double AOI;

    int msgNum = 1;//表示节点编号

    int node_num = 700;//表示所有节点的个数, 300以前 6

    string result = "./results/Baseline/nodeNum=";//表示结果的存放地址

    double transmissionRange = 300;//表示节点的数据包的发送范围

    vector<double>v_period;//用来保存一个周期内所有节点的发送周期
    vector<double>v_aoi;//用来保存一个周期内的所有节点的AoI

    vector<vector<simtime_t>>v_send;//将一个周期内节点收到的所有数据包的发送时间按照源节点的编号进行分类保存，用于计算不同节点对的平均AoI
    vector<vector<simtime_t>>v_recv;//将一个周期内节点收到的所有数据包的接收时间按照源节点的编号进行分类保存，用于计算不同节点对的平均AoI
    vector<vector<double>>v_Period;//将一个周期内收到的所有数据包的源节点的发送间隔按照源节点的编号进行分类保存，用来计算不同节点对的平均周期
    vector<double>avgAoi;//

    set<int> myset;

    TimePointType m_StartTimerPointer;
    TimePointType endTimerPointer;
    veins::TimerManager timerManager{this};

protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;//定义初始化消息的模块
    virtual void handleMessage(cMessage *msg) override;//定义处理消息的模块

    void sendMessage();//定义发送消息的模块
    void updatePeriod();//定义更新发送周期的模块
    void initSet();//定义清空所有容器的模块
    void calculateAoi();//定义计算Aoi的模块
    void vectorInit();//定义初始化所有容器的模块
    void printAoi();
};

#endif
