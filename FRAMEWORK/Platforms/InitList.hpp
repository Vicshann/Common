
#pragma once

// Has to be in STD, it is bolted down
// initializer_list has a variable length; the compiler takes care of that
//
namespace std
{

    template<typename T>
    class initializer_list
    {
    private:
        const T* m_first;
        const T* m_last;

    public:
        using value_type      = T;
        using reference       = const T&;
        using const_reference = const T&;
        using size_type       = decltype(sizeof(void*));
        using iterator        = const T*;
        using const_iterator  = const T*;

        constexpr initializer_list() noexcept : m_first(nullptr), m_last(nullptr) {}

        // Number of elements.
        constexpr size_type size() const noexcept { return m_last - m_first; }

        // First element.
        constexpr const T* begin() const noexcept { return m_first; }

        // One past the last element.
        constexpr const T* end() const noexcept { return m_last; }
    };

/*template<typename T> class initializer_list
{
public:
    using value_type = T;
    using reference  = const T&;
    using const_ref  = const T&;
    using size_type  = decltype(sizeof(void*));  // Type of sizeof is size_t
    using iterator   = const T*;
    using const_itr  = const T*;

private:
    const_itr m_first;
    const_itr m_last;   // Not actually last but first beyond
    size_type m_len;

    // The compiler can call a private constructor.
    constexpr initializer_list(const_itr itr, size_type st): m_first(itr), m_last(itr+st), m_len(st) { }    // GCC

public:
    constexpr initializer_list() noexcept : m_first(nullptr), m_last(nullptr), m_len(0) {}

    constexpr initializer_list(const_itr first, const_itr last) noexcept : m_first(first), m_last(last), m_len(last-first) {}   // MSVC

    constexpr size_type size() const noexcept { return m_last - m_first; }    // Number of elements.

    constexpr const_itr begin() const noexcept { return m_first; }            // First element.

    constexpr const_itr end() const noexcept { return m_last; }               // One past the last element.
};
  */
//------------------------------------------------------------------------------------------------------------
template<typename T> constexpr const T* begin(initializer_list<T> il) noexcept {return il.begin();}
template<typename T> constexpr const T* end(initializer_list<T> il) noexcept{return il.end();}
//------------------------------------------------------------------------------------------------------------
}
