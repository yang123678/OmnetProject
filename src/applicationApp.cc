#include "applicationApp.h"
#include "MyPacket_m.h"

Define_Module(ApplicationApp);

void ApplicationApp::initSet()//用来清空每一轮实验的所有容器和初始化参数
{
    this->v_send.clear();
    this->v_recv.clear();
    this->v_Period.clear();
    this->v_aoi.clear();
    this->v_period.clear();
    this->avgAoi.clear();
    this->t_send = 0;
    this->t_recv = 0;
    this->myset.clear();
}

void ApplicationApp::vectorInit()//在所有的二维数组中插入一维数组,形成空的二维数组
{
    for(int i = 0;i < this->node_num;i++)
        {
            this->v_send.push_back(vector<simtime_t>());
            this->v_recv.push_back(vector<simtime_t>());
            this->v_Period.push_back(vector<double>());
        }
    this->msgNum = 1;
}

void ApplicationApp::initialize(int stage){
    srand((unsigned) time(0)); //初始化随机数种子
    cSimpleModule::initialize(stage);
    if (stage != INITSTAGE_APPLICATION_LAYER) return;

    localPort_ = par("localPort").intValue();
    size_ = par("PacketSize");

    //初始化所有要用到的参数
    this->r = 1.1;
    this->aoi_last = 0;
    this->t_send = 0;
    this->t_recv = 0;


    //调用初始化容器的函数,先清空所有容器，然后重新生成二维数组
    vectorInit();

    //随机生成初始发送周期
    std::default_random_engine e(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> u(0.1, 0.1);
    this->period__ = u(e);//随机生成第一个时间段内的发送周期

    cout<<getParentModule()->getIndex()<<":"<<this->period__;
    cout<<endl;

    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);
//    socket.setBroadcast(true);

    //利用组播实现广播的功能
    IInterfaceTable *ift = getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
    MulticastGroupList mgl = ift->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);
    // if the multicastInterface parameter is not empty, set the interface explicitly
    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        InterfaceEntry *ie = ift->findInterfaceByName(multicastInterface);
        if (!ie) throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    sendPacket = new cMessage("sendPacket");//生成第一个数据包
    scheduleAt(simTime()+this->period__, sendPacket);//发送第一个数据包

    m_StartTimerPointer = std::chrono::high_resolution_clock::now();//记录第一个数据包开始发送的时刻

    auto callback = [this]() {//从第6秒开始，每隔6秒更新一次发送周期和系统平均AoI

        cancelAndDelete(sendPacket);//取消上一轮的自消息

        cout<<"-----------------当前节点为：" << getParentModule()->getIndex() << "---------------" << endl;
        updatePeriod();//更新发送周期并且计算平均AoI
        printAoi();

        initSet();//清空所有容器
        vectorInit();//重新创建需要的容器

        sendPacket = new cMessage("sendPacket");
        scheduleAt(simTime()+this->period__, sendPacket);//初始化本轮实验的第一个数据包

        endTimerPointer = std::chrono::high_resolution_clock::now();
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimerPointer).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimerPointer).time_since_epoch().count();

        if(getParentModule()->getIndex()==1)
        {
            ofstream ofs;
            ofstream ofs1;
            ofstream ofs2;
            ofs.open(result + to_string(this->node_num) + "/Execution_Time.txt",ios::out|ios::app);
            ofs1.open(result + to_string(this->node_num) + "/Recv_Msg.txt",ios::out|ios::app);
            ofs<< start << "," << end <<endl;
            ofs1<<0<<","<<0<<","<<0<<endl;
            ofs2.open(result + to_string(this->node_num) + "/Time_Delay.txt",ios::out|ios::app);
            ofs2<<simTime()<<","<<endl;
        }
    };
    timerManager.create(veins::TimerSpecification(callback).oneshotAt(SimTime(16, SIMTIME_S)).interval(SimTime(1, SIMTIME_S)).repetitions(5));
}

void ApplicationApp::handleMessage(cMessage *msg){
    if(msg->isSelfMessage())
    {
        if(msg == sendPacket){
            sendMessage();
            scheduleAt(simTime() + this->period__, sendPacket);
        }else{
            timerManager.handleMessage(msg);
        }
    }
    else
    {
        Packet* pPacket = check_and_cast<Packet*>(msg);
        auto recvPacket = pPacket->popAtFront<MyPacket>();

        Coord position;
        double distance = 0;

        mobility = check_and_cast<inet::IMobility*>(getParentModule()->getSubmodule("mobility"));
        position = mobility->getCurrentPosition();

        distance = ((position.x - recvPacket->getXSend()) * (position.x - recvPacket->getXSend())
                + (position.y - recvPacket->getYSend()) * (position.y - recvPacket->getYSend()));

        distance = sqrt(distance);

        if(distance <= this->transmissionRange) {
//            if(recvPacket->getSourceId() == 1) {//只存储1号节点发来的数据包
            ofstream ofs;
            ofs.open(result + to_string(this->node_num) + "/Recv_Msg.txt",ios::out|ios::app);
            ofs<<recvPacket->getSourceId()<<","<<(getParentModule()->getIndex())<<","<<recvPacket->getMsgNum()<<endl;

//                ofstream ofs1;
//                ofs1.open("/home/veins/workspace.omnetpp/baseline_aoi/out/result/nodeNum=" + to_string(this->node_num) + "/Time_Delay.txt",ios::out|ios::app);
//                ofs1<<recvPacket->getMsgNum()<<","<<(simTime()-recvPacket->getTime())<<endl;
//            }

            this->myset.insert(getParentModule()->getIndex());

            this->v_send[recvPacket->getSourceId()].push_back(recvPacket->getSendTime());//将接收到的数据包的发送时间按照源节点分类保存到对应的vector容器中
            this->v_recv[recvPacket->getSourceId()].push_back(simTime());//将收到的数据包的接收时间按照源节点分类保存到对应的容器中
            this->v_Period[recvPacket->getSourceId()].push_back(recvPacket->getPeriod());//将收到的数据包的源节点的发送周期按照元节点编号保存到不同的容器中


            if(recvPacket->getMsgNum() == 30) {
                ofstream ofs2;
                ofs2.open(result + to_string(this->node_num) + "/Time_Delay.txt",ios::out|ios::app);
                ofs2<<"Delay:" << simTime() - recvPacket->getSendTime() <<endl;
            }
        }

//        if(this->myset.size() == this->node_num) {
//            ofstream ofs1;
//            ofs1.open(result + to_string(this->node_num) + "/Time_Delay.txt",ios::out|ios::app);
//            ofs1<<simTime()<<endl;
//        }

        delete pPacket;
        mobility = nullptr;
    }
}

void ApplicationApp::sendMessage()
{
    Packet *packet = new inet::Packet("Send_state");
    Coord position;
    const auto& payload = makeShared<MyPacket>();

    mobility = check_and_cast<inet::IMobility*>(getParentModule()->getSubmodule("mobility"));
    position = mobility->getCurrentPosition();

    payload->setSourceId(getParentModule()->getIndex());
    payload->setSendTime((simTime()-this->period__));
    payload->setTime(simTime());
    payload->setPeriod(this->period__);
    payload->setChunkLength(B(size_));
    payload->setXSend(position.x);
    payload->setYSend(position.y);
    payload->setMsgNum(this->msgNum);

    if(getParentModule()->getIndex() == 1) {//只存储1号节点发送的数据包
        ofstream ofs;
        ofs.open(result + to_string(this->node_num) + "/Send_Msg.txt",ios::out|ios::app);
        ofs<<(getParentModule()->getIndex())<<","<<this->msgNum<<endl;
    }

    packet->insertAtBack(payload);

    L3Address destAddress = L3AddressResolver().resolve("224.0.0.1");
    socket.sendTo(packet, destAddress, localPort_);

    mobility = nullptr;
    this->msgNum += 1;

//    if(this->msgNum == 30) {
//        ofstream ofs2;
//        ofs2.open(result + to_string(this->node_num) + "/Time_Delay.txt",ios::out|ios::app);
//        ofs2<< this->msgNum <<",send:"<< simTime() <<endl;
//    }
}

void ApplicationApp::calculateAoi()
{
    double Aoi;
    double sumPeriod=0;
    double periodavg = 0;

    if(this->v_send.size() != 0) {
        for(unsigned int i = 0;i < this->v_send.size();i++)//外层循环实现对不同的节点对进行遍历 # 越界
        {
            Aoi = 0;
            double time = 0;
            if(this->v_send[i].size() != 0) {
                for(unsigned int j = 0;j < this->v_send[i].size()-1;j++)//内层实现对不同的节点对计算平均Aoi
                {
                    if((this->v_send[i][j+1] - this->v_recv[i][j]).dbl() <= 0) {
                        Aoi = Aoi + ((0.5*((this->v_recv[i][j+1]-this->v_send[i][j])).dbl()*((this->v_recv[i][j+1]-this->v_send[i][j])).dbl())
                                -0.5*(((this->v_recv[i][j]-this->v_send[i][j]).dbl())*((this->v_recv[i][j]-this->v_send[i][j]).dbl())));
                    } else {
                        Aoi = Aoi + (0.5*((this->v_recv[i][j]-this->v_send[i][j])).dbl()*((this->v_recv[i][j]-this->v_send[i][j])).dbl());
                    }
                }
        //        cout<<Aoi<<endl;
                time = (this->v_recv[i][this->v_recv[i].size()-1] - this->v_send[i][0]).dbl();
                this->v_aoi.push_back(Aoi/time);//将计算后的每个节点对的平均Aoi存入容器中,目前由于有些节点发送的数据包收不到，所以会导致时间差过大，因此计算出的Aoi值很大
            }
        }

    #if 1
        for(unsigned int k = 0;k < this->v_Period.size();k++)//遍历不同节点对的源节点发送周期
        {
            sumPeriod = 0;
            if(this->v_Period[k].size() != 0) {
                for(unsigned int l = 0;l < this->v_Period[k].size();l++)
                {
                    sumPeriod = sumPeriod + this->v_Period[k][l];
                }
                periodavg=sumPeriod/(this->v_Period[k].size());
                this->v_period.push_back(periodavg);//将计算后的每个节点对的发送周期存到容器中，目前会报错，
                                                     //因为在一个周期内的不是所有的节点发送的数据都可以收到，也就是说有的节点对为空
            }
        }
    }
#endif
}

void ApplicationApp::printAoi()
{
    double sumAoi = 0;
    this->AOI = 0;
    if(this->avgAoi.size() != 0) {
        for(int i = 0;i < this->avgAoi.size();i++) {
            sumAoi = sumAoi + this->avgAoi[i];
        }
        this->AOI = sumAoi / this->avgAoi.size();
        ofstream ofs;
        ofs.open(result + to_string(this->node_num) + "/AoI_msg.txt",ios::out|ios::app);
        ofs<<getParentModule()->getIndex()<<","<<AOI<<endl;

        ofstream ofs1;
        ofs1.open(result + to_string(this->node_num) + "/Period_msg.txt",ios::out|ios::app);
        ofs1<<getParentModule()->getIndex()<<","<<this->period__<<endl;
    }
}

void ApplicationApp::updatePeriod()
{
    double period_sum=0;
    double period_avg=0;
    double period_s=0;
    simtime_t aoi_sum=0;
    simtime_t aoi_avg=0;
    simtime_t new_aoi = 0;
    simtime_t prew_aoi = 0;
    simtime_t aoi_diff = 0;
    double new_period = 0;
    simtime_t new_aoi_diff = 0;
    simtime_t normalized_gradient = 0;
    simtime_t aoi_low = 0.01;
    simtime_t aoi_high = 0.1;
    simtime_t minAoI = 0.05;
    double period_minus = 0.001;
    double a = 0.125;
    double b = 0.8;
    int n = 5;

    calculateAoi();

    if(this->v_period.size() != 0) {
        for(unsigned int i=0 ; i < this->v_period.size() ; i++)
        {
            cout << "源节点周期为：" << this->v_period[i] << " aoi为：" << this->v_aoi[i] << endl;
            period_sum += this->v_period[i];
            aoi_sum += this->v_aoi[i];
        }
        period_avg = period_sum/(this->v_period.size());
        aoi_avg = aoi_sum.dbl() / (this->v_aoi.size());

        this->avgAoi.push_back((aoi_avg).dbl());

// One-Hop-20230405
//        this->period__ += 0.005;


//Baseline-20221201
        if(period_avg < 0.1)
        {
            period_s = period_avg / 2;
        }else{
            period_s = 0.05;
        }

        if(abs(period_avg - this->period__) > period_s)
        {
            this->period__ = period_avg;
        }
        else{
            if(aoi_avg.dbl() > 2*period_avg)
            {
                this->period__ = this->period__*this->r;
            }else if(aoi_avg > this->aoi_last){
                this->period__ = this->period__/this->r;
            }
        this->aoi_last = (aoi_avg).dbl();
        }


//Timely-20230403
//        new_aoi = aoi_avg;
//        new_aoi_diff = new_aoi - prew_aoi;
//        prew_aoi = new_aoi;
//        aoi_diff = (1-a) * aoi_diff + a * new_aoi_diff;
//        normalized_gradient = (aoi_diff).dbl() / minAoI;
//        new_period = this->period__;
//
//
//        if(new_aoi < aoi_low) {
//            this->period__ = this->period__ - period_minus;
//        } else if (new_aoi > aoi_high) {
//            this->period__ = this->period__ * (1 - b * (1 - aoi_high / new_aoi));
//        } else if (normalized_gradient <= 0) {
//            this->period__ = this->period__ - n * period_minus;
//        } else {
//            this->period__ = this->period__ * (1 - b * (normalized_gradient).dbl());
//        }
    }
}
