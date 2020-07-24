#ifndef MIR_CLIENT_H
#define MIR_CLIENT_H

class MIR_Client {
public:
    MIR_Client();
    virtual ~MIR_Client();

    bool StartRunClientThread(int nHpNo, int nMirroringPort, int nControlPort);

private:
};

#endif