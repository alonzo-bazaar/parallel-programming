# bqn methlab to prototype/fuck around with the brent kung adder schema
# before trying to implement it in opencl

# comments below written in unholy mix of bqn, latex, numpy&Co.
# sorry

# so brent kung goes like 
# first pass (here arbitrairily called "converging")
# iw := "indices where"
# a[iw %2=1] += a[iw %2=0]
# a[iw %4=3] += a[iw %4=1]
# a[iw %8=7] += a[iw %8=3]
# ...
# a[iw %(2⋆n)=(1-˜2⋆n)] += a[iw %(2⋆n)=(1-˜ 2⋆n-1)]
# up until 2^n > ≠a

# second pass (here arbitrairily called "distribution")
# assuming array of length 9, where the greatest while power of 2 below ≠a is 8
# a[iw %8=7] +=˜ a[(iw %8=7) +4]
# a[iw %4=3] +=˜ a[(iw %4=3) +2]
# a[iw %2=1] +=˜ a[(iw %2=1) +1]
# so the general step is
# stride ← 2⋆n
# source ← ((stride-1)=stride|⊢)⊸/ ↕≠a
# dst ← source + stride÷2
# a[src] += a[dst]

# add to self at indices
# returns a but with a[si] += a[di]
ASI ← {si‿di𝕊a : (si+○(⊏⟜a)di)⌾(di⊸⊏) a }

# Brent kung convergint step
# performs one step in brent kung's converging phase
BKCS ← { s𝕊a: # s = stride, a = array
              # returns a but indices where (1-˜ 2⋆n)   = (2⋆n)|i 
              # are +='d with indices where (1-˜ 2⋆n-1) = (2⋆n)|i
              # where the current stride s←2⋆n-1
    ss ← 2×s                   # convenience variable
    di ← ((1-˜ss)=ss⊸|)⊸/ ↕≠a  # destination indices 
    si ← di-s                  # source indices 
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

# brent kung distribution phase (see above, same shit)
# brent kung distribution step
BKDS ← { s𝕊a:
     ss ← 2×s
     di ← (≠a)⊸>⊸/ s + ((1-˜ss)=ss⊸|)⊸/ ↕≠a
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

# some utils for fucking around with the above code
nl ← ≍@+10                  # newline
JL ← ∾´ ∾⟜nl¨               # join lines
DH ← {•Out JL ⥊⟜'#'¨ 𝕩 ⋄ @} # display histogram

# testing our implementation of bk
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
