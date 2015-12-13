#ifndef FALCON_REGEX_DFA_CONSUMER_HPP
#define FALCON_REGEX_DFA_CONSUMER_HPP

#include <iosfwd>
#include <cstdint>

namespace falcon { namespace regex_dfa {

    using std::size_t;
    using char_int = uint32_t;

    struct utf8_char
    {
        explicit utf8_char(char_int c)
        : uc(c)
        {}

        char_int uc;
    };

    template<class Ch, class Tr>
    std::basic_ostream<Ch, Tr> &
    operator<<(std::basic_ostream<Ch, Tr> & os, utf8_char utf8_c)
    {
        char c[] = {
            char((utf8_c.uc & 0xff000000) >> 24),
            char((utf8_c.uc & 0x00ff0000) >> 16),
            char((utf8_c.uc & 0x0000ff00) >> 8),
            char((utf8_c.uc & 0x000000ff)),
        };
        if (c[0]) {
            return os.write(c, 4);
        }
        else if (c[1]) {
            return os.write(c+1, 3);
        }
        else if (c[2]) {
            return os.write(c+2, 2);
        }
        else {
            os.write(c+3, 1);
        }
        return os;
    }


    class utf8_consumer
    {
    public:
        explicit utf8_consumer(const char * str)
        : s(reinterpret_cast<const unsigned char *>(str))
        {}

        char_int bumpc()
        {
            char_int c = *this->s;
            if (!c) {
                return c;
            }
            ++this->s;
            if (*this->s >> 6 == 2) {
                c <<= 8;
                c |= *this->s;
                ++this->s;
                if (*this->s >> 6 == 2) {
                    c <<= 8;
                    c |= *this->s;
                    ++this->s;
                    if (*this->s >> 6 == 2) {
                        c <<= 8;
                        c |= *this->s;
                        ++this->s;
                    }
                }
            }
            return c;
        }

        char_int getc() const
        {
            return utf8_consumer(this->str()).bumpc();
        }

        const char * str() const
        {
            return reinterpret_cast<const char *>(s);
        }

        void str(const char * str)
        {
            s = reinterpret_cast<const unsigned char *>(str);
        }

        const unsigned char * s;
    };

} }

#endif
