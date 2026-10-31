<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.11.0" name="tileset_anim" tilewidth="32" tileheight="32" tilecount="16" columns="4">
 <!-- Scarab's tile animation test tileset (doc/MASTER_PLAN.md item V13, sunlight I11): Scarab's monkey icons, one
      folder up. Tile 0 animates over three icons for 100, 50 and 300 ms (a 450 ms cycle; 50 ms is not a whole
      number of 60 fps frames); tile 3 is an animation whose durations are all 0 (it shows its first frame); tile 15
      has no animation. -->
 <image source="../images/monke_variants.png" width="128" height="128"/>
 <tile id="0">
  <animation>
   <frame tileid="0" duration="100"/>
   <frame tileid="5" duration="50"/>
   <frame tileid="10" duration="300"/>
  </animation>
 </tile>
 <tile id="3">
  <animation>
   <frame tileid="3" duration="0"/>
   <frame tileid="6" duration="0"/>
  </animation>
 </tile>
</tileset>
