#pragma once

struct UnloadT {
    bool features{ };
    bool visuals{ };
    bool should_unload{ };
};

extern UnloadT g_unload;
