#import "@preview/typslides:1.3.4": *
#show: typslides.with(
  ratio: "16-9",
  theme: "dusky",
  font: "Fira Sans",
  font-size: 20pt,
  link-style: "color",
  show-progress: true,
)

#front-slide(
  title: "Boids in C and OpenMP",
  subtitle: "Parallelizing particle simulation",
  authors: "H.Kirollos",
  info: [#link("https://github.com/alonzo-bazaar/parallel-programming")],
)

#table-of-contents()

#focus-slide[Boids]
#slide(title:"Boids", outlined: true)[
  - Boids (short for bird-oids, or just birds pronounced with a new york accent) are buncha bird like objects
  - behaviour of one object determined by other objects within a certain radius
  - alignment, go near, go far behaviour
]
#slide(title:"Procedure")[
  boids initially made as an alternative to force field based approaches, as more complex bodies whose behaviour was determined by controls given to a bird-oid system to follow certain rules, but given that force field based methods have the same parallel aspects as the more advanced bird-oid approach, we have opted to develop one such system instead

  procedure is
  - for all boids
    - if it's too close to the edge or out of bounds add a force to it to go back in bounds
    - for all other boids ($O(n^2)$)
      - if it's within a certain distance then add a force to the boid to be repelled (not colliding)
      - if it's within a certain distance then add a force to the boid to be go in a similar direction
      - if it's within a certain distance then add it as a neighbour
    - add a force to the boid to move closer to the average between all neighbour nodes
]
#slide(title:"Our Procedure, and Parallel Patterns Therein")[
  - one buffer of boids
  - for all boids compute boid update and write boid update to other boid buffer (map)
  - (optional) display boids
  - swap buffers so written boid buffer becomes current iteration, and old boid buffer is where we're gonna write the next iteration to
]
#focus-slide[Implementation]
#slide(title:"Techonlogies used", outlined: true)[
  sequential and parallel versions implemented using c and openmp 
  given openmp's nature as pragmas to "parallelize this array" it was possible to use the same code for the sequential and parallel version, and switch between the two by just turning the openmp pragmas on and off by means of appropriate compiler flags
]
#slide(title:"Versions implemented")[
  implmented soa and aos versions
  implemented graphical version to see the thing (unit test a occhio) and benchmark version to see how fast it go
]
#slide(title:"Benchmarks", outlined: true)[
  amount of warmup iterations
  amount of counted iterations
  done
]
#slide(title:"Benchmarks Results")[
]
#slide(title:"Conclusions")[
]
