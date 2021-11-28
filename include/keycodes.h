//
// Created by Бушев Дмитрий on 13.11.2021.
//

#ifndef RENDERAPITEST_KEYCODES_H
#define RENDERAPITEST_KEYCODES_H

namespace APITest {
enum Key {
#define KEY(LITERAL) KEY_##LITERAL = #LITERAL[0],
  KEY(A) KEY(B) KEY(C) KEY(D) KEY(E) KEY(F) KEY(G) KEY(H) KEY(I) KEY(J) KEY(K)
      KEY(L) KEY(M) KEY(N) KEY(O) KEY(P) KEY(Q) KEY(R) KEY(S) KEY(T) KEY(U)
          KEY(V) KEY(W) KEY(X) KEY(Y) KEY(Z)
#undef KEY
#define KEY(NAME) KEY_##NAME
              KEY(SEMICOLON) = ';',
  KEY(QUOTE) = '\'',
  KEY(APOSTROPHE) = '`',
  KEY(L_BRACKET) = '[',
  KEY(R_BRACKET) = ']',
  KEY(COMMA) = ',',
  KEY(POINT) = '.',
  KEY(SLASH) = '/',
  KEY(BACKSLASH) = '\\',
#undef KEY
#define KEY(NUMERIC) KEY_NUM_##NUMERIC = NUMERIC,
  KEY(0) KEY(1) KEY(2) KEY(3) KEY(4) KEY(5) KEY(6) KEY(7) KEY(8) KEY(9)
#undef KEY
#define KEY(NUM_PAD) KEY_NUM_PAD_##NUM_PAD,
      KEY(0) KEY(1) KEY(2) KEY(3) KEY(4) KEY(5) KEY(6) KEY(7) KEY(8) KEY(9)
#undef KEY
#define KEY(FUNC) KEY_F##FUNC,
          KEY(1) KEY(2) KEY(3) KEY(4) KEY(5) KEY(6) KEY(7) KEY(8) KEY(9) KEY(10)
              KEY(11) KEY(12)
#undef KEY
#define KEY(NAME) KEY_##NAME
                  KEY(TAB),
  KEY(CAPS_LOCK),
  KEY(LEFT_SHIFT),
  KEY(RIGHT_SHIFT),
  KEY(LEFT_CONTROL),
  KEY(RIGHT_CONTROL),
  KEY(LEFT_ALT),
  KEY(RIGHT_ALT),
  KEY(SPACE),
  KEY(ENTER),
  KEY(BACKSPACE),
  KEY(INSERT),
  KEY(DELETE),
  KEY(HOME),
  KEY(END),
  KEY(PAGE_UP),
  KEY(PAGE_DOWN),
  KEY(NUM_PAD_ADD),
  KEY(NUM_PAD_SUBTRACT),
  KEY(NUM_PAD_DIVIDE),
  KEY(NUM_PAD_MULTIPLY),
  KEY(NUM_PAD_ENTER),
  KEY(NUM_PAD_DECIMAL),
  KEY(NUM_PAD_EQUAL),
  KEY(MINUS),
  KEY(EQUAL),
  KEY(ESCAPE),
  KEY(LEFT),
  KEY(RIGHT),
  KEY(UP),
  KEY(DOWN),
  KEY(UNKNOWN),
};

#undef KEY

static_assert(KEY_B == static_cast<Key>('B'));
static_assert(KEY_NUM_3 == static_cast<Key>(3));

} // namespace APITest
#endif // RENDERAPITEST_KEYCODES_H
