## 8503 - Game Technology - 2020/2021
<br />
This repo shows physics and AI implementations for the 8503 coursework at Newcastle University.
<br />

### Acknowledgements
The main framework was provided by Dr Rich Davison and he has given permission to use it for learning purposes.
<br /><br />

### Short Demonstration Video
[New Game mode]()<br />
[Practice mode]()
<br /><br />

### Screenshots
<a name = "raycast"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/main.gif" width = "400"></a> 
<br /><br />

#### Raycasting
_Raycasting are done against spheres, AABB, OBB and capsules. The ray is casted from the mouse pointer towards a point on the object. Debug info is shown when the object is clicked on._

<a name = "raycast"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/raycast.PNG" width = "400"></a> 
<br /><br />

#### Object Collision Detection and Response
**_Implemented:_**

- _AABB vs AABB_
- _Sphere vs Sphere_
- _AABB/OBB vs Sphere_
- _Capsule vs Sphere_
- _Capsule vs OBB_

_Angular and linear impulses have been implemented. Objects are able to rotate away at the point of collision using torque.
Different coefficient of restitutions have been used for various objects so their velocities will differ during collisions._ 

<a name = "capSph"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/capsuleSphere.gif" width = "400"></a> 
<a name = "sphereAABB"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/sphereAABB.gif" width = "400"></a> 
<a name = "obbCapSph"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/obbCapSph2.gif" width = "400"></a> 
<br /><br />

#### Pathfinding 
_The AI uses A* pathfinding to work it's way to the finishing point. If the blue AI sees a coin it will go after it. If the player has a pink coin, the AI will attempt to chase after player if they are close enough. <br />
The AI recalculates a new path if it deviates from its original and it will teleport behind the player if it gets stuck._

<a name = "ai"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/aiMovement.gif" width = "400"></a> 
<br /><br />

#### Constraints
_The bridge consists of two ends having blocks of infinite masses. The middle blocks are connected together with a constraint. If one block moves, the others are affected as well.<br />
The purple ball is constraint to a block floating in the air and is imitating a ball on the end of a rope._

<a name = "bridge"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/bridge.PNG" width = "400"></a> 
<a name = "constraint"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/constraint.gif" width = "400"></a> 
<br /><br />

#### Pushdown Automata
_The menu is made with a pushdown automata which is a stack of states. When a menu option is chosen it will get pushed onto the state stack and whichever state is on top of the stack will get executed. Upon pressing Escape the state will get popped off the top of the stack._

<a name = "menu"><img src="https://github.com/Akeilee/Game-Tech/blob/main/Screenshots/start.PNG" width = "400"></a> 
<br /><br />

### Game Modes
**- New Game :** Plays against AI. Loses when health drops to 0 and wins if crosses finish line with health more than 0.<br />
**- Practice Mode :** Unlimited life and can test object collisions and movements.<br /><br />
<br />

### Keybinds
**Menu**
|||
| :---: | :---: |
|**Key**|**Description**|
|**W,S  /  Up, Down Arrow**| Move between selections |
|**Enter**| Run selected menu option |
|**Escape**| Exits game |

**General**
|||
| :---: | :---: |
|**Key**|**Description**|
|**P**| Pause game |
|**U**| Unpause game |
|**F1**| Reset simulation |
|**G**| Toggles gravity |
|**Q**| Toggles mouse pointer (enables objects to be clicked) |
|**L**| Toggles locked object (when object is clicked on) |
|**Escape**| Back to main menu |
|**NumPad 0**| Toggles player mode and camera mode |
|**0**| Toggle display for AI pathfinding |

**Practice Mode**
|||
| :---: | :---: |
|**9**| Toggle AI movement |

**Camera Mode**
|||
| :---: | :---: |
|**W,S,A,D, Mouse**| Moving camera |
|**Shift, Spacebar**| Moving camera up and down |

**Player Mode**
|||
| :---: | :---: |
|**W,S,A,D, Spacebar**| Moves character and jump |
|**1,3**| Rotates character |

**Selected/Locked Object Mode**
|||
| :---: | :---: |
|**Up, Down, Left, Right Arrow**| Moves object |
|**NumPad 2,4,6,8**| Rotates object on different axis (selected object only) |
|**NumPad 5**| Move object on Y axis (selected object only) |
|**Mouse scroll and click**| After selecting the object, you can manually apply different forces to the object at certain points |
<br />

