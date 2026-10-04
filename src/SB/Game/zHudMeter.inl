// Retail places the HUD meter query after the font-meter helper group.
// Keep this definition private to zHud; other callers retain xHudMeter.h's body.
inline U32 xhud::meter_widget::changing() const
{
    return (value == end_value) ^ 1;
}
