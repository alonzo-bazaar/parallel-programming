#!/usr/bin/env bqn

# there was a cleaner way to do this, I forgot it
SplitBy ← (({¯1¨⌾(𝕩⊸/)+`𝕩}=) ⊔ ⊢)

# pardon the abstraction, latent java brain
# well, c++, but let's say java
_lpadder ← {𝕨(⊢∾𝕗⥊˜⊣-≠∘⊢)𝕩}
_rpadder ← {𝕨(⊢∾˜𝕗⥊˜⊣-≠∘⊢)𝕩}
LSpcPad ← " "_lpadder
RSpcPad ← " "_rpadder

# lines
ls ← •FLines "/dev/stdin"

# split lines
ss ← ','⊸SplitBy¨ls

# max lengths of each column
mcls ← ⌈˝ > ≠¨¨ ss
padding←2

•Out¨ ∾⟜" "⊸∾´¨ (mcls+padding)⊸(Lspcpad¨)¨ ss

