#include "partition_manager.hpp"
#include <iostream>

int main() {
    std::cout << "====================================================================\n";
    std::cout << "   AVIONICS DUAL-BANK (A/B) FLASH FAILOVER & INTEGRITY MANAGER     \n";
    std::cout << "   Safety-Critical Subsystem Architecture (Teledyne ECP Model)     \n";
    std::cout << "   Author: Amit Bakale (Embedded Software Engineer)                \n";
    std::cout << "====================================================================\n\n";

    avionics::PartitionManager mgr;

    // Phase 1: Initialize dual partitions with 18 subsystems
    std::cout << ">>> PHASE 1: SYSTEM INITIALIZATION & 18 SUBSYSTEM PROVISIONING\n";
    mgr.initialize("/dev/mtdblock_avionics");
    std::cout << "\n";

    // Phase 2: First boot sequence
    std::cout << ">>> PHASE 2: NORMAL INITIAL FLIGHT BOOT SEQUENCE\n";
    mgr.boot_sequence();
    std::cout << "Current State: " << mgr.get_status_summary() << "\n\n";

    // Phase 3: Simulated inflight power cut during partition write
    std::cout << ">>> PHASE 3: SIMULATING IN-FLIGHT POWER LOSS & PARTITION CORRUPTION\n";
    mgr.simulate_inflight_power_dropout();
    std::cout << "\n";

    // Phase 4: Reboot after power drop (triggers MD5 audit failure and automated A/B fallback)
    std::cout << ">>> PHASE 4: SYSTEM REBOOT & AUTOMATED ZERO-TOUCH FAILOVER\n";
    bool boot_ok = mgr.boot_sequence();
    if (boot_ok) {
        std::cout << "\n[SUCCESS] System recovered without maintenance intervention!\n";
        std::cout << "Current State: " << mgr.get_status_summary() << "\n";
    } else {
        std::cerr << "\n[FATAL] System failed to recover!\n";
    }

    // Phase 5: Print detailed safety audit log
    mgr.print_system_audit_log();

    return boot_ok ? 0 : 1;
}
