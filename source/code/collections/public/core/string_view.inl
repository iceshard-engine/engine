//! \brief Creates a new StackString object with the given value.
template<typename CharType>
constexpr core::StringView<CharType>::StringView(CharType const* cstring) noexcept
    : _size{ core::cstring::length(cstring) }
    , _data{ cstring }
{
}

template<typename CharType>
constexpr core::StringView<CharType>::StringView(CharType const* beg, CharType const* end) noexcept
    : _size{ static_cast<uint32_t>(std::distance(beg, end)) }
    , _data{ beg }
{
}

//! \brief Creates a new StackString object with the given value.
template<typename CharType>
template<typename size_t Size>
constexpr core::StringView<CharType>::StringView(CharType const (&cstring)[Size]) noexcept
    : _size{ Size }
    , _data{ cstring }
{
}

//! \brief Creates a new StackString object with the given value.
template<typename CharType>
core::StringView<CharType>::StringView(std::string const& str) noexcept
    : _size{ static_cast<uint32_t>(str.size()) }
    , _data{ str.data() }
{
}

//! \brief Creates a new StackString object with the given value.
template<typename CharType>
constexpr core::StringView<CharType>::StringView(std::string_view str_view) noexcept
    : _size{ static_cast<uint32_t>(str_view.size()) }
    , _data{ str_view.data() }
{
}

//! \brief Creates a new StringView from a String.
template<typename CharType>
core::StringView<CharType>::StringView(core::String<CharType> const& other) noexcept
    : _size{ other._size }
    , _data{ other._data }
{
}

//! \brief Creates a new StringView from a StackString.
template<typename CharType>
template<uint32_t Capacity>
core::StringView<CharType>::StringView(core::StackString<Capacity, CharType> const& other) noexcept
    : _size{ other._size }
    , _data{ other._data }
{
}

//! \brief Replaces the string value with the new one.
template<typename CharType>
auto core::StringView<CharType>::operator=(CharType const* other) noexcept -> StringView&
{
    _size = strlen(other);
    _data = other;
    return *this;
}

//! \brief Replaces the string value with the new one.
template<typename CharType>
auto core::StringView<CharType>::operator=(std::string_view other) noexcept -> StringView&
{
    _size = other.size();
    _data = other.data();
    return *this;
}

//! \brief Replaces the string value with the new one.
template<typename CharType>
template<uint32_t Capacity>
auto core::StringView<CharType>::operator=(StackString<Capacity, CharType> const& other) noexcept -> StringView&
{
    _size = other._size;
    _data = other._data;
    return *this;
}

//! \brief Replaces the string value with the new one.
//! \details If the input String is larger, it will only copy the maximum
//!     amount of characters the rest will be discarded.
template<typename CharType>
auto core::StringView<CharType>::operator=(String<CharType> const& other) noexcept -> StringView&
{
    _size = other._size;
    _data = other._data;
    return *this;
}

//! \brief Returns the character at the given position.
template<typename CharType>
constexpr auto core::StringView<CharType>::operator[](uint32_t i) const noexcept -> const CharType&
{
    return _data[i];
}

//! \brief Size of the string.
template<typename CharType>
inline auto core::string::size(const core::StringView<CharType>& str) noexcept -> uint32_t
{
    return str._size;
}

//! \brief Length of the string.
template<typename CharType>
auto core::string::length(const core::StringView<CharType>& str) noexcept -> uint32_t
{
    return str._size - 1;
}

//! \brief The current string capacity.
template<typename CharType>
auto core::string::capacity(const core::StringView<CharType>& str) noexcept -> uint32_t
{
    return core::string::size(str);
}

//! \brief Checks if the given string is empty.
template<typename CharType>
bool core::string::empty(const core::StringView<CharType>& str) noexcept
{
    return str._size == 0 || *str._data == '\0';
}

//! \copydoc core::string::begin(const String<CharType>&)
template<typename CharType>
auto core::string::begin(const core::StringView<CharType>& str) noexcept -> const CharType*
{
    return str._data;
}

//! \copydoc core::string::end(const String<CharType>&)
template<typename CharType>
auto core::string::end(const core::StringView<CharType>& str) noexcept -> const CharType*
{
    return str._data + str._size;
}

//! \copydoc core::string::front(String<CharType>&)
template<typename CharType>
auto core::string::front(const core::StringView<CharType>& str) noexcept -> const CharType&
{
    return str._data[0];
}

//! \copydoc core::string::back(String<CharType>&)
template<typename CharType>
auto core::string::back(const core::StringView<CharType>& str) noexcept -> const CharType&
{
    return str._data[str._size - 1];
}

//! \brief Clears the string view object.
template<typename CharType>
void core::string::clear(core::StringView<CharType>& str) noexcept
{
    str._size = 0;
    str._data = nullptr;
}

template<typename CharType>
bool core::string::equals(const core::StringView<CharType>& left, const core::StringView<CharType>& right) noexcept
{
    return equals(left, right._data);
}

template<typename CharType>
bool core::string::equals(const core::StringView<CharType>& left, const core::String<CharType>& right) noexcept
{
    return equals(left, right._data);
}

template<uint32_t Capacity, typename CharType>
bool core::string::equals(const core::StringView<CharType>& left, const core::StackString<Capacity, CharType>& right) noexcept
{
    return equals(left, right._data);
}

template<typename CharType>
bool core::string::equals(const core::StringView<CharType>& left, const std::string_view right) noexcept
{
    return equals(left, right.data());
}

template<typename CharType>
bool core::string::equals(const core::StringView<CharType>& left, const CharType* right) noexcept
{
    return strcmp(left._data, right) == 0;
}
