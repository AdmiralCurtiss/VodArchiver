#pragma once

#include "i-status-update.h"

namespace VodArchiver {
struct NullStatusUpdate : public IStatusUpdate {
    ~NullStatusUpdate() override;
    void Update() override;
};
} // namespace VodArchiver
