#pragma once

namespace VodArchiver {
struct IStatusUpdate {
    virtual ~IStatusUpdate();
    virtual void Update() = 0;
};
} // namespace VodArchiver
