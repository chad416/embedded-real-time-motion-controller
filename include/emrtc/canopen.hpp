#pragma once

#include "emrtc/adapter_config.hpp"
#include "emrtc/bus.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace emrtc {

struct CanFrame {
    std::uint32_t id{0};
    std::uint8_t dlc{0};
    std::array<std::uint8_t, 8> data{};
};

enum class CanReceiveStatus {
    Frame,
    Timeout,
    Error
};

class ICanTransport {
public:
    virtual ~ICanTransport() = default;
    virtual bool open(const std::string& interface_name, std::string& detail) = 0;
    virtual void close() = 0;
    virtual bool send(const CanFrame& frame, std::string& detail) = 0;
    virtual CanReceiveStatus receive(CanFrame& frame,
                                     std::uint32_t timeout_us,
                                     std::string& detail) = 0;
    virtual bool is_open() const = 0;
};

class SocketCanTransport final : public ICanTransport {
public:
    SocketCanTransport();
    ~SocketCanTransport() override;

    SocketCanTransport(const SocketCanTransport&) = delete;
    SocketCanTransport& operator=(const SocketCanTransport&) = delete;

    bool open(const std::string& interface_name, std::string& detail) override;
    void close() override;
    bool send(const CanFrame& frame, std::string& detail) override;
    CanReceiveStatus receive(CanFrame& frame,
                             std::uint32_t timeout_us,
                             std::string& detail) override;
    bool is_open() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

CanFrame encode_canopen_rpdo1(const ProcessImage& image,
                              const SocketCanCanOpenConfig& config);
CanFrame encode_canopen_rpdo2(const ProcessImage& image,
                              const SocketCanCanOpenConfig& config);
AdapterContractResult decode_canopen_tpdo(const CanFrame& frame,
                                          const SocketCanCanOpenConfig& config,
                                          ProcessImage& image,
                                          bool& received_tpdo1,
                                          bool& received_tpdo2);

class SocketCanCanOpenBackend final : public IBusBackend {
public:
    SocketCanCanOpenBackend();
    explicit SocketCanCanOpenBackend(ICanTransport& transport);
    ~SocketCanCanOpenBackend() override;

    SocketCanCanOpenBackend(const SocketCanCanOpenBackend&) = delete;
    SocketCanCanOpenBackend& operator=(const SocketCanCanOpenBackend&) = delete;

    AdapterConfigurationResult configure(const SocketCanCanOpenConfig& config);
    void shutdown();
    BusExchangeResult exchange(CycleIndex cycle,
                               std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;
    AdapterStatus status() const;

private:
    AdapterConfigurationResult fail_configuration(const std::string& detail);
    bool handle_receive_frame(const CanFrame& frame,
                              CycleIndex cycle,
                              ProcessImage& image,
                              bool& received_tpdo1,
                              bool& received_tpdo2);

    std::unique_ptr<ICanTransport> owned_transport_;
    ICanTransport* transport_{nullptr};
    SocketCanCanOpenConfig config_;
    BusDiagnostics diagnostics_;
    AdapterStatus status_;
    bool injected_transport_{false};
    bool have_heartbeat_{false};
    bool previous_online_{false};
};

}  // namespace emrtc
