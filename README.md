3D-Tiles
---
Working on a demo of fast tiled 3D map rendering using glMultiDrawElementsIndirect and some other methods.
Think maps like the levels in [Captain Toad](https://www.youtube.com/watch?v=m91qkP5ZaN8) where it's tiled but
tiles can have different geometry and aren't necessarily uniform in size.

Current Rendering Status
---
The tiles are drawn with a single multi draw indirect call.

![Some tiles drawn with one multi draw indirect call](http://i.imgur.com/tie04Bq.png)

