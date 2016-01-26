package org.processwarp.android;

public class Worker {
    /**
     * Delegate for implement by service.
     */
    public interface Delegate {
        void workerRelayCommand(Worker caller, CommandPacket packet);
    }

    /** Delegate instance for pass packet. */
    private Delegate delegate;
    /** Process-id bundled for process. */
    private String myPid;
    /** Node-id of this node. */
    private String myNid;

    /**
     * Initialize worker and create VM by native method.
     * @param delegate Delegate for implement by service.
     * @param myNid Node-id of this node.
     * @param myPid Process-id for new vm.
     * @param rootTid Root thread-id for new vm.
     * @param procAddr Address of process information for new vm.
     * @param masterNid Node-id of master node for new vm.
     */
    public void initialize(Delegate delegate, String myNid, String myPid,
                           long rootTid, long procAddr, String masterNid) {
        this.delegate = delegate;
        this.myNid = myNid;
        this.myPid = myPid;

        workerInitialize(this, myNid, myPid, rootTid, procAddr, masterNid);
    }

    /**
     * Get Process-id bundled for process.
     * @return Process-id bundled for process.
     */
    public String getPid() {
        return myPid;
    }

    /**
     * When receive packet from another node or another module to worker, relay it by NDK.
     * @param pid Process-id bundled to packet.
     * @param dstNid Destination node-id.
     * @param srcNid Source node-id, should be my node-id, THIS, or NONE.
     * @param module Target module.
     * @param content Packet content string of JSON.
     */
    public void relayWorkerCommand(String pid, String dstNid, String srcNid,
                                   int module, String content) {
        workerRelayCommand(pid, dstNid, srcNid, module, content);
    }

    /**
     * When worker require send a packet, relay command by command type.
     * @param pid Process-id bundled to packet.
     * @param dstNid Destination node-id.
     * @param srcNid Source node-id, should be my node-id, THIS, or NONE.
     * @param module Target module.
     * @param content Packet content string of JSON.
     */
    private void workerSendCommand(String pid, String dstNid, String srcNid,
                                   int module, String content) {
        CommandPacket packet = new CommandPacket();
        packet.pid = pid;
        packet.dstNid = dstNid;
        packet.srcNid = srcNid;
        packet.module = module;
        packet.content = content;
        delegate.workerRelayCommand(this, packet);
    }

    private native void workerInitialize(Worker worker, String myNid, String myPid, long rootTid,
                                         long procAddr, String masterNid);
    private native void workerRelayCommand(String pid, String dstNid, String srcNid,
                                           int module, String content);
    private native void workerQuit(String pid);
}