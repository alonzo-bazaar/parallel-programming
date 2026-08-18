# brent kung, but now different indices computation

# utils
nl ← ≍@+10                  # newline
JL ← ∾´ ∾⟜nl¨               # join lines
DH ← {•Out JL ⥊⟜'#'¨ 𝕩 ⋄ @} # display histogram

# business logic
# add to self at indices
# returns a but with a[si] += a[di]
ASI ← {si‿di𝕊a : (si+○(⊏⟜a)di)⌾(di⊸⊏) a }

# a[iw %2=1] += a[iw %2=0] # dispari 0 volte, dispari 1 volta
# a[iw %4=3] += a[iw %4=1] # dispari 1 volta, dispari 2 volte
# a[iw %8=7] += a[iw %8=3] # ...
# ...
# a[iw %(2⋆n)=(1-˜2⋆n)] += a[iw %(2⋆n)=(1-˜ 2⋆n-1)]
# up until 2^n > ≠a
BKCS ← { s𝕊a: # s = stride, a = array
              # returns a but indices where (1-˜ 2⋆n)   = (2⋆n)|i 
              # are +='d with indices where (1-˜ 2⋆n-1) = (2⋆n)|i
              # where the current stride s←2⋆n-1
    ss ← 2×s
    di ← 1-˜ ss× 1+ ↕⌊ss÷˜≠a  # (%ss = 1-˜ss)
    si ← di-s                 # (%ss = 1-˜ s)
    si‿di ASI a
}

# brent kung converging
# repeat brent kung converging step with stride from 1 till its larger 
# than array size, multiplying stride by 2 every time

# NOTE: cbqn doesn't have tco
# but... we don't need that much stack
# and I find this way of writing thigs clearer
BKC ← {
    𝕊a  : 1𝕊a ;
    s𝕊a : s > ≠a ? a ;
    s𝕊a : s ≤ ≠a ? (s×2) 𝕊 s BKCS a
}

# a[iw %8=7] +=˜ a[(iw %8=7) +4]
# a[iw %4=3] +=˜ a[(iw %4=3) +2]
# a[iw %2=1] +=˜ a[(iw %2=1) +1]
# in general
# stride ← 2⋆n
# source ← ((stride-1)=stride|⊢)⊸/ ↕≠a
# dst ← source + stride÷2
# a[src] += a[dst]
BKDS ← { s𝕊a:
     ss ← 2×s
     # old version:
     # di ← (≠a)⊸>⊸/ s + ((1-˜ss)=ss⊸|)⊸/ ↕≠a
     # si ← di-s

     di ← (≠a)⊸>⊸/ s+ 1-˜ ss× 1+ ↕⌊ss÷˜≠a
     si ← di-s
     si‿di ASI a
}

# brent kung distribution
BKD ← {
    𝕊a  : is ← 2÷˜⌊⌾(2⊸⋆⁼) ≠a # initial stride
          is𝕊a ;
    s𝕊a : s ≤ 0 ? a ;
    s𝕊a : s > 0 ? (⌊s÷2) 𝕊 s BKDS a
}

# brent kung
BK ← BKD BKC

# testing
TST ← {(BK𝕩) ≡ (+`𝕩)}
! TST 0 ⥊ 0
! TST 10 ⥊ 0
! TST 10 ⥊ 1
! TST 7 ⥊ 1
! TST 8 ⥊ 1
! TST 9 ⥊ 1
! TST ↕10
! TST ↕20
! TST ↕100
