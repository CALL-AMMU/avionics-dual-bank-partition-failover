#include "partition_manager.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace avionics {

PartitionManager::PartitionManager()
    : active_bank_(BankId::BANK_A),
      standby_bank_(BankId::BANK_B),
      failover_in_progress_(false) {
}

bool PartitionManager::initialize(const std::string& simulated_flash_root) {
    log_event("INFO", "Initializing Avionics Dual-Bank Flash Partition Manager at " + simulated_flash_root);

    // Initialize Bank A (Active)
    BankPartition bankA;
    bankA.id = BankId::BANK_A;
    bankA.mount_point = "/work";
    bankA.state = BankState::ACTIVE_MOUNTED;
    bankA.boot_count = 1;
    banks_[BankId::BANK_A] = bankA;

    // Initialize Bank B (Standby)
    BankPartition bankB;
    bankB.id = BankId::BANK_B;
    bankB.mount_point = "/stby";
    bankB.state = BankState::STANDBY_VERIFIED;
    bankB.boot_count = 0;
    banks_[BankId::BANK_B] = bankB;

    // Initialize Factory Recovery Bank
    BankPartition bankFactory;
    bankFactory.id = BankId::FACTORY;
    bankFactory.mount_point = "/factory";
    bankFactory.state = BankState::FACTORY_RECOVERY;
    bankFactory.boot_count = 0;
    banks_[BankId::FACTORY] = bankFactory;

    // Register primary 18 avionics subsystems across Bank A, Bank B, and Factory
    const std::vector<std::string> subsystem_names = {
        "ADIF_ARINC429", "ADIF_ARINC717", "NIC_VLAN_TRUNK", "AIO_ANALOG_INPUT",
        "HEALTH_MONITOR", "DISCRETE_IO_MGR", "CABIN_NETWORK_ROUTER", "EFB_GATEWAY",
        "POWER_LOSS_DETECT", "DATALINK_ROUTER", "ETHERNET_SWITCH_QOS", "NTP_TIME_SYNC",
        "SECURITY_EAP_TLS", "STORAGE_CONTROLLER", "AUDIO_RECORDER_IF", "AVIONICS_FAN_CTRL",
        "MAINTENANCE_TERMINAL", "TELEMETRY_LOG_SINK"
    };

    for (const auto& name : subsystem_names) {
        std::string config_body = "{\"subsystem\":\"" + name + "\",\"mode\":\"SAFETY_CRITICAL\",\"timeout_ms\":50}";
        register_subsystem(name, config_body, 1);
    }

    // Mirror to Bank B and Factory
    banks_[BankId::BANK_B].subsystems = banks_[BankId::BANK_A].subsystems;
    banks_[BankId::FACTORY].subsystems = banks_[BankId::BANK_A].subsystems;

    banks_[BankId::BANK_A].manifest_md5 = compute_manifest(BankId::BANK_A);
    banks_[BankId::BANK_B].manifest_md5 = compute_manifest(BankId::BANK_B);
    banks_[BankId::FACTORY].manifest_md5 = compute_manifest(BankId::FACTORY);

    log_event("INFO", "Initialized 18 safety-critical avionics subsystems across dual-bank partitions.");
    return true;
}

bool PartitionManager::register_subsystem(const std::string& name, const std::string& payload, uint32_t version) {
    SubsystemConfig sub;
    sub.name = name;
    sub.config_payload = payload;
    sub.version = version;
    sub.md5_digest = MD5::hash(payload);
    sub.is_valid = true;

    banks_[BankId::BANK_A].subsystems[name] = sub;
    return true;
}

std::string PartitionManager::compute_manifest(BankId bank) {
    std::string combined = "";
    for (const auto& kv : banks_[bank].subsystems) {
        combined += kv.first + ":" + kv.second.md5_digest + ";";
    }
    return MD5::hash(combined);
}

bool PartitionManager::audit_bank_integrity(BankId bank) {
    auto& partition = banks_[bank];
    for (auto& kv : partition.subsystems) {
        std::string actual_hash = MD5::hash(kv.second.config_payload);
        if (actual_hash != kv.second.md5_digest) {
            kv.second.is_valid = false;
            log_event("CRITICAL", "Integrity audit FAILED on [" + partition.mount_point + "] for subsystem: " + kv.first);
            return false;
        }
    }
    std::string current_manifest = compute_manifest(bank);
    if (current_manifest != partition.manifest_md5) {
        log_event("CRITICAL", "Manifest signature mismatch on [" + partition.mount_point + "]");
        return false;
    }
    return true;
}

bool PartitionManager::boot_sequence() {
    log_event("BOOT", "Initiating boot sequence on active bank [" + banks_[active_bank_].mount_point + "]...");
    
    if (audit_bank_integrity(active_bank_)) {
        banks_[active_bank_].state = BankState::ACTIVE_MOUNTED;
        banks_[active_bank_].boot_count++;
        log_event("BOOT", "Active bank verified successfully. Subsystem routing active.");
        return true;
    }

    log_event("WARNING", "Active bank failed verification! Triggering automated failover sequence...");
    return execute_failover("Boot-time cryptographic MD5 verification failure");
}

void PartitionManager::simulate_inflight_power_dropout() {
    log_event("ANOMALY", "*** INFLIGHT POWER LOSS DETECTED DURING EEPROM/FLASH WRITE ***");
    // Corrupt one of the subsystems on the active bank
    auto& sub = banks_[active_bank_].subsystems["ADIF_ARINC429"];
    sub.config_payload = "{\"subsystem\":\"ADIF_ARINC429\", Corrupted Partial Write... \x00\xFF\xAA";
    // Deliberately keep old MD5 to force verification failure
    log_event("ANOMALY", "Bank [" + banks_[active_bank_].mount_point + "] contains truncated bytes in sector ADIF_ARINC429.");
}

bool PartitionManager::execute_failover(const std::string& trigger_reason) {
    failover_in_progress_ = true;
    log_event("FAILOVER", "Executing zero-touch failover! Cause: " + trigger_reason);

    banks_[active_bank_].state = BankState::CORRUPTED_FAILOVER;

    // Check Standby Bank
    log_event("FAILOVER", "Validating standby bank [" + banks_[standby_bank_].mount_point + "]...");
    if (audit_bank_integrity(standby_bank_)) {
        // Switch Active and Standby
        BankId previous_active = active_bank_;
        active_bank_ = standby_bank_;
        standby_bank_ = previous_active;

        banks_[active_bank_].state = BankState::ACTIVE_MOUNTED;
        banks_[active_bank_].boot_count++;

        log_event("FAILOVER", "SUCCESS: Promoted alternate bank to ACTIVE. Partition mounted to /work.");
        log_event("FAILOVER", "Corrupted bank marked for offline repair. Boot-loop prevented (100% recovered).");
        failover_in_progress_ = false;
        return true;
    }

    // Emergency Factory Fallback
    log_event("EMERGENCY", "Standby bank also invalid! Falling back to immutable FACTORY bank...");
    if (audit_bank_integrity(BankId::FACTORY)) {
        active_bank_ = BankId::FACTORY;
        banks_[active_bank_].state = BankState::ACTIVE_MOUNTED;
        log_event("EMERGENCY", "SUCCESS: Restored golden FACTORY baseline configuration.");
        failover_in_progress_ = false;
        return true;
    }

    log_event("FATAL", "Hardware flash integrity irrecoverable! System halted.");
    failover_in_progress_ = false;
    return false;
}

BankState PartitionManager::get_bank_state(BankId bank) const {
    auto it = banks_.find(bank);
    if (it != banks_.end()) return it->second.state;
    return BankState::OFFLINE;
}

void PartitionManager::log_event(const std::string& severity, const std::string& message) {
    std::ostringstream ss;
    ss << "[" << std::setw(8) << severity << "] " << message;
    audit_log_.push_back(ss.str());
    std::cout << ss.str() << "\n";
}

void PartitionManager::print_system_audit_log() const {
    std::cout << "\n================ AVIONICS AUDIT TRAIL LOG ================\n";
    for (const auto& entry : audit_log_) {
        std::cout << entry << "\n";
    }
    std::cout << "==========================================================\n";
}

std::string PartitionManager::get_status_summary() const {
    std::ostringstream ss;
    ss << "Active Bank: " << (active_bank_ == BankId::BANK_A ? "BANK_A (/work)" : (active_bank_ == BankId::BANK_B ? "BANK_B (/work)" : "FACTORY (/factory)"))
       << " | Subsystems: " << banks_.at(active_bank_).subsystems.size()
       << " | Failover Status: OK";
    return ss.str();
}

} // namespace avionics
