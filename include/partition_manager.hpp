#ifndef PARTITION_MANAGER_HPP
#define PARTITION_MANAGER_HPP

#include "md5.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace avionics {

enum class BankId {
    BANK_A = 0,
    BANK_B = 1,
    FACTORY = 2
};

enum class BankState {
    ACTIVE_MOUNTED,
    STANDBY_VERIFIED,
    CORRUPTED_FAILOVER,
    FACTORY_RECOVERY,
    OFFLINE
};

struct SubsystemConfig {
    std::string name;             // e.g. "ADIF_ARINC429", "NIC_VLAN", "HEALTH_MGR"
    std::string config_payload;    // JSON/Text configuration content
    std::string md5_digest;        // Cryptographic integrity signature
    uint32_t version;
    bool is_valid;
};

struct BankPartition {
    BankId id;
    std::string mount_point;       // "/work", "/stby", "/factory"
    BankState state;
    uint32_t boot_count;
    std::map<std::string, SubsystemConfig> subsystems;
    std::string manifest_md5;
};

class PartitionManager {
public:
    PartitionManager();
    ~PartitionManager() = default;

    // Initialization & Lifecycle
    bool initialize(const std::string& simulated_flash_root);
    bool boot_sequence();

    // Configuration & Subsystems
    bool register_subsystem(const std::string& name, const std::string& payload, uint32_t version);
    bool commit_configuration(BankId target_bank);

    // Fail-safe & Anomaly Simulation
    void simulate_inflight_power_dropout();
    bool audit_bank_integrity(BankId bank);
    bool execute_failover(const std::string& trigger_reason);

    // Accessors & Diagnostics
    BankId get_active_bank() const { return active_bank_; }
    BankState get_bank_state(BankId bank) const;
    void print_system_audit_log() const;
    std::string get_status_summary() const;

private:
    std::string compute_manifest(BankId bank);
    void log_event(const std::string& severity, const std::string& message);

    BankId active_bank_;
    BankId standby_bank_;
    std::map<BankId, BankPartition> banks_;
    std::vector<std::string> audit_log_;
    bool failover_in_progress_;
};

} // namespace avionics

#endif // PARTITION_MANAGER_HPP
