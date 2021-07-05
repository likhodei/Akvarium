// No header guard
#if defined (_MSC_VER)
//#pragma warning(push) // Save warning settings.
# pragma warning (disable:4103)
# pragma warning (disable: 4996) // Disable deprecated std::swap_ranges, std::equal
# pragma pack (push, 8)
#elif defined (__BORLANDC__)
# pragma option push -a8 -b -Ve- -Vx- -w-rvl -w-rch -w-ccc -w-obs -w-aus -w-pia -w-inl -w-sig
# pragma nopushoptwarn
# pragma nopackwarning
#endif
