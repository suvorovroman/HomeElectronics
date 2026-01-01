struct udiv8_t
{
  uint8_t quot;
  uint8_t rem;
  udiv8_t(){}
  udiv8_t(uint8_t _q, uint8_t _r):quot(_q), rem(_r){}
};

struct udiv8_t udiv(uint16_t _n, uint8_t _d)
{
  return udiv8_t(_n/_d, _n%_d);
}
