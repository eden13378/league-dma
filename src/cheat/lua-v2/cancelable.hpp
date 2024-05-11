#pragma once

namespace lua {
    class CancelableAction {
    public:
        auto cancel( ) -> void{ m_should_cancel = true; }

        [[nodiscard]] auto should_cancel( ) const -> bool{ return m_should_cancel; }

    private:
        bool m_should_cancel{ };
    };
}
