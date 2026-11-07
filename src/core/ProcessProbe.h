#pragma once
#include <QString>
#include <QtGlobal>
#include <optional>

enum class State : quint8 { Stopped, Starting, Ready, Orphan };

struct Snapshot {
    State state = State::Stopped;
    int pid = 0;
    bool engine = false;
    bool hasHealth = false;
    QString model;
    int maxContext = 0;
    bool loaded = false;
    QString uptime;
};

struct GpuMem {
    double usedGb = 0.0, totalGb = 0.0;
    bool ok = false;
    int percent() const;
};

namespace Probe {
bool procAlive(int pid);
int portOwner(int port);
bool engineRunning();
int pidFromFile();
QString procUptime(int pid);
std::optional<GpuMem> gpuMem();
Snapshot probeLocal();
QString tailLog(int lines);
QString baseNameOf(const QString &path);
}  // namespace Probe
