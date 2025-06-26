#pragma once

namespace VodArchiver {
struct IStatusUpdate {
    virtual ~IStatusUpdate() = default;
    virtual void Update() = 0;
};
} // namespace VodArchiver
