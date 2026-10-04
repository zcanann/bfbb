// TU-private definitions preserve the retail xCamera inline section.

inline xVec2& xVec2::operator=(F32 f)
{
    this->x = this->y = f;

    return *this;
}
