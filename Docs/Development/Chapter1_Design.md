# Chapter 1 · The Compass: the developer's design

Version 1.0 · 2026-11-14 · Claude as the game's designer, the author as the player.

The author asked me to put his own ideas aside, act as the developer, research the scientific way to design a first
chapter, and walk him through the result in words. **How I read "put my ideas aside":** the design calls in this
chapter are mine. I keep what already exists in the game and works (the real Everest from Dingboche, Sangye, the
parents, the compass), because a developer does not throw away a working foundation; everything else here is my
decision, made from the research in section 1. This document replaces Chapter 1 in `Chapter_Design_v1.md`.

Contents: 1 what the research says · 2 the chapter at a glance · 3 the teaching ladder · 4 the emotional curve ·
**5 the walkthrough, in words** · 6 what must be built · 7 how we test it · 8 sources.

---

## 1. What science and practice say about a first chapter

| # | Finding | Evidence | Rule for Chapter 1 |
|---|---|---|---|
| 1 | **The first minutes decide.** Retention curves either stay flat in the opening minutes or fall minute by minute; the first ten minutes are the most important, and they should show what the game really is | Google Play's analysis of early-session retention; reviewers' "first ten minutes" tradition | The player has control within 30 seconds, and within the first 5 minutes feels the real game: thin air, a rope, wind, fear |
| 2 | **Up-front tutorials barely help; in-context help does.** In a study of over 45,000 players, tutorials raised play time by up to 29% only in the most complex game and had a negligible effect in simpler ones; players learn more by experimenting than by reading, and help given just before it is needed beats a manual | Andersen et al., CHI 2012 | No tutorial screens. Each thing is explained in one line, at the moment it is needed, by a person in the world when possible |
| 3 | **Teach without words.** Super Mario Bros. World 1-1 teaches jumping, enemies and power-ups without a line of text: the level is shaped so the player discovers each rule safely | Miyamoto's breakdown of World 1-1 and its analyses | The village is shaped to teach: steep stone stairs teach breathing, a practice rock teaches the rope |
| 4 | **One thing at a time.** Working memory is small; teach one concept per moment and let it be mastered before the next | Cognitive load theory as applied to onboarding | One new mechanic per scene, never two |
| 5 | **Four steps per idea.** Nintendo builds each idea in four steps: introduce it safely, develop it, twist it, conclude it (kishōtenketsu, used by Koichi Hayashida from Super Mario Galaxy on) | Game Maker's Toolkit on Super Mario 3D World; Hayashida interviews | Every mechanic in the chapter follows introduce → develop → twist → conclude (section 3) |
| 6 | **Flow: challenge must match skill,** neither anxious nor bored; the best adjustment is a choice the player makes, not a hidden dial | Csikszentmihalyi; Jenova Chen, *Flow in Games* (2006) | The chapter is gentle, but the skilled can choose harder lines (the crack on the practice rock) |
| 7 | **Start in the middle, but show normal life too.** Opening *in medias res* skips exposition and hooks; its cost is that players never see what normal looked like before things went wrong | design writing on *in medias res* openings (Uncharted 2, Planescape: Torment) | A four-minute flash-forward to summit night, then "22 days earlier": an ordinary afternoon in Dingboche |
| 8 | **Spaces tell stories.** Environmental storytelling (Don Carson, from theme-park design) and Henry Jenkins's narrative architecture: rooms and objects evoke and embed a story the player pieces together | Carson (2000); Jenkins, "Game Design as Narrative Architecture" | The parents' room tells their story through objects; nobody explains it |
| 9 | **A bond can be made by a mechanic.** In *Ico*, holding Yorda's hand, a held button, builds attachment without words; Fumito Ueda's "design by subtraction" keeps the cast tiny so every person matters | *Ico* and Ueda's design talks | A child takes Sangye's hand: the same held input later means a rope partner holding you, and in the best ending, Sangye holding his mother's hand. Four named people in the chapter, no more |
| 10 | **People remember the peak and the end** (the peak-end rule, Kahneman and colleagues) more than the length | peak-end research; design practice | The chapter has one designed peak (dusk, the compass, the burning palm) and one designed end (the farewell and the first journal page) |
| 11 | **Open loops pull** (the Zeigarnik effect): an unanswered question stays on the mind | Zeigarnik; *Glued to Games* (Rigby, Ryan) | The prologue's question ("who called his name at 8,500 m?") and the chapter's last question stay open |
| 12 | **Autonomy, competence, relatedness** make play enjoyable and good for the player | Ryan, Rigby, Przybylski (2006) | Real choices (what to pack, the truth or a lie), a first mastery (the clip), and people who care (the aunt, the child, the uncle) |

---

## 2. The chapter at a glance

| | |
|---|---|
| Length | **44 minutes**: a 4-minute prologue, then 40 minutes in Dingboche |
| Place | the death zone at night (prologue); Dingboche village, the gompa above it, a practice rock by the river (positions [Open]: to be measured on the real terrain) |
| Season and time | spring, the climbing season. The afternoon of day 1, 13:00 → night, with two short time jumps |
| People | **Aunt Pema** (runs the teahouse), **Dolma** (her daughter, eight), **Uncle Pasang** (retired climbing Sherpa, lost four toes to frostbite twenty years ago), **the lama** at the gompa. Dr. Hale is only a letter |
| Taught, one at a time | look and walk · read things · hold a hand · the oximeter · breathing and the rest-step · the compass · clipping to a rope · the pack and its weight · the journal |
| The chapter's question | *The lama says they are not dead. Then what are they?* |
| The peak | dusk on the roof: the plume above the Lhotse wall turns gold, the compass needle swings to the hidden summit, and the mark in Sangye's palm burns with his heartbeat |
| The end | the aunt's farewell, Dolma asleep holding his hand, the first journal page writing itself |

---

## 3. The teaching ladder

One mechanic per scene, each in four steps (finding 5). Minutes from the start of the prologue.

| Mechanic | Introduce (safe) | Develop | Twist | Conclude | Min |
|---|---|---|---|---|---|
| *Prologue: a taste, not a lesson* | walk on a rope in the wind | — | the regulator freezes | a slip into darkness | 0-4 |
| Look and walk | the parents' quiet room | climb the attic ladder | a loose board creaks under you | open the box | 4-7 |
| Read things | a photo on the wall | the route map with a red line | the line stops at 8,500 m | the letters in the box | 5-10 |
| Hold a hand | Dolma takes your hand | she leads you down the lane | she stops at a yak in the way; you wait together | she lets go at the teahouse door | 10-11 |
| The oximeter | clip it on: 84% | the number dips when you stand up fast | a trekker's reads 79%, "and I feel fine" | it stays on the screen from now on | 12-14 |
| Breathing and the rest-step | the stone stairs to the gompa make your breath loud | the hint appears when the breath is loudest: hold to rest-step | the stairs steepen; sprinting empties your breath | the top, breath calming | 18-21 |
| The compass | open it: the needle points north | turn: it holds north | on the gompa roof it swings away from north, towards the hidden summit | the palm mark burns once | 24-27 |
| Clipping | flat ledge, three anchors, clip and unclip | a 30° slab, five anchors | mitts on (the cold is coming): clips are slower; miss one and you slip, and the uncle's rope catches you | top out at the prayer flags | 27-36 |
| The pack | open it: the weight bar | add things, watch the bar | past 16 kg your walk slows | choose three extras, close it | 36-41 |
| The journal | — | — | — | Sangye writes the first page | 43-44 |

---

## 4. The emotional curve

| Min | Scene | Feeling | Tension (1-5) | Company |
|---|---|---|---|---|
| 0-4 | Prologue: summit night | fear, awe, a mystery | 5 | alone (and a voice) |
| 4-10 | The parents' house | quiet, guilt, curiosity | 2 | alone |
| 10-11 | Dolma's hand | warmth | 1 | a child |
| 11-17 | The teahouse | belonging, worry | 2 | crowd |
| 17-24 | The gompa | effort, unease | 3 | the lama |
| 24-27 | The compass on the roof | wonder, a chill | 3 | alone |
| 27-36 | The practice rock | concentration, a scare, pride | 3-4 | the uncle |
| 36-41 | Packing | choosing, care | 2 | the family |
| 41-43 | **Dusk: the peak** | awe, dread, resolve | 4 | Dolma |
| 43-44 | **Night: the end** | love, farewell, a question | 2 | the family, then sleep |

---

## 5. The walkthrough, in words

*This is what you, the player, experience. Lines in grey brackets are notes from me, the developer, about choices and
what they change.*

### Prologue · Twenty-two days from now (4 minutes)

Black screen. No logo, no menu yet. You hear it before you see it: wind, a huge steady roar, like standing behind a
jet engine. Under it, close, a hiss and a click, a hiss and a click: someone breathing through an oxygen mask.

Light. A small white circle: your headlamp. It shows snow streaming sideways, a rope running up into the dark, and
your own glove gripping a metal clamp on it. Your breathing is the loudest thing in the world.

You can move. **[Control within the first 20 seconds: finding 1.]** You push forward and the boots bite into hard snow
with a crunch you feel more than hear. At the edge of the screen, in thin white numbers, the only interface: **8,512 m**
and **SpO₂ 61%**. Your heartbeat thuds behind everything.

The rope runs up a ridge so narrow that the headlamp shows nothing on either side. Just black. Every fifteen metres or
so the rope is tied to an anchor screwed into the ice. When you reach the first one, a single word appears: *Clip.* You
press it; the carabiner snaps over with a metallic click, and for a moment the sound is the safest thing you have ever
heard.

Two anchors further, the hiss in the mask stops.

The number starts falling. 61. 58. 55. The heartbeat speeds up, the edges of the view darken and narrow like a tunnel.
A word: *Knock the regulator.* You press it; Sangye bangs the frozen valve against his ice axe, twice, three times, and
the hiss comes back. The number climbs again, slowly. You realise you were holding your own breath.

Then, ahead, at the very edge of the headlamp, someone is standing on the ridge. A woman, no mask, no headlamp, a
pale shape in the spindrift. She does not move. Through the wind, quite clearly, she says your name.

*"Sangye."*

You take one step towards her. The snow under your boot breaks.

The world tips. You slide. A word flashes: *Axe!* Whether you press it in time or not, the screen goes white, then
black, and the only thing left is the heartbeat, slowing.

Text, small, centred:

**Twenty-two days earlier.**

**[Design: the prologue promises the whole game in four minutes: the death zone, the rope, oxygen, a body under
strain, and the myth (the woman). It ends on an open question (finding 11) and is followed by normal life (finding
7). It teaches nothing the player must remember: every action is prompted once.]**

### Scene 1 · The parents' house (13:00, 6 minutes)

A yak bell, far away. Birds. Afternoon sun through a small dusty window.

You are standing in a low room with whitewashed stone walls. Sangye's breath is quiet now. The oximeter is gone from
the corner of the screen; there is nothing on the screen at all. You are home, in Dingboche, in the house your parents
left twenty years ago. Your aunt has kept it swept, but nobody lives here.

You look around. The room tells you who lived here without anyone saying a word. **[Environmental storytelling:
finding 8.]**
- On the wall, two pale outlines in the soot where two ice axes used to hang. Only the hooks are left.
- A framed photograph: a man and a woman in faded down suits, laughing, frost in their eyebrows, a boy of about six
  between them refusing to look at the camera. You.
- Pinned to the wall, a big hand-drawn map of the route from Base Camp to the summit, covered in your mother's small,
  neat writing. A red pencil line climbs it camp by camp, and stops at 8,500 m. Next to the end of the line, one word,
  pressed so hard the pencil tore the paper: *Here.*
- A child's drawing, in crayon, of three people on top of a triangle. Two of them are scribbled out.

When you look at something, a soft outline shows you can pick it up. You turn the photo over; on the back, in your
father's hand: *"He'll understand one day."*

A wooden ladder leads to the attic. Climbing it is just walking forward; you notice the climb in your arms on screen.
**[Walk, look, read: one at a time.]** Up there, under a slipping tarp, a tin box. One board creaks loudly under your
foot and you stop by reflex. Nothing happens. It is only an old house. **[A twist with no danger: the player learns
the world reacts.]**

In the box: letters tied with string, a woollen hat, and wrapped in a yak-wool sock, something heavy and round. A
brass compass, old, its lid engraved all over with sharp angular marks that are not Tibetan and not English. You do
not open it yet; the game does not make you. **[The compass is introduced by its weight, not its function.]**

The letters are your parents' last ones, from Base Camp. You can read them all or none. The last line of the last
letter, in your mother's hand: *"Tell Sangye we'll explain everything when we're down."*

The door bangs open downstairs.

### Scene 2 · Dolma's hand and the teahouse (14:00, 7 minutes)

*"Sangye! Mama says come and eat!"*

Dolma, your cousin, eight years old, red cheeks, a hat with ear flaps. She grabs your hand without asking. A small
icon of two hands appears with the word *Hold.* When you hold the button, she leads you; when you let go, she stops
and looks back at you, puzzled, until you take her hand again. **[Finding 9: a bond made by a held input. The same
input is used much later when Nima short-ropes you down a slope, and in the best ending when Sangye holds his
mother's hand.]**

She pulls you down a lane between stone walls built from flat grey rocks. A yak stands across the lane, chewing,
completely uninterested. Dolma stops. You wait together. The yak moves on. She says, very seriously, "Always stand on
the mountain side when a yak comes." **[Taught in a safe place, for the yak trains on the trail in Chapter 2.]**

The teahouse: warm, low, smoky, and full. A stove in the middle burning dried yak dung, a big copper kettle, two
trekkers from Germany in expensive jackets, a porter asleep in the corner. The sound changes completely: voices,
laughter, the kettle, a radio playing Nepali pop. After the empty house, the room feels like a hug.

Aunt Pema puts a bowl of potatoes and chilli sauce in front of you, then an envelope with a foreign stamp. It is from
**Dr. Ingrid Hale**, Base Camp: a typed letter welcoming you to her physiology study ("you will be the only participant
born at 4,000 m, which makes you very interesting to me") and, in a small plastic case, a **finger oximeter**.

*Clip it on.* You do. The number appears at the edge of the screen and stays there for the rest of the game:
**SpO₂ 84%, pulse 72.** Pema laughs: "In Kathmandu it would say 98. Here everybody is 84. Up there..." She does not
finish. **[The oximeter is introduced at rest, in a safe warm room, with a person explaining it in one sentence:
finding 2.]**

If you stand up quickly, the number dips to 82 and climbs back. One of the German trekkers leans over: "Can I? Mine is
79 and I feel totally fine." Pema gives him a look you will understand in Chapter 2.

Then Pema sits down opposite you and asks, quietly: "Why are you really going up there?"

**[Decision. Two answers:**
**· The truth** ("They never came down. I want to know why."): she is silent, then takes a white scarf, a khata, from
the shelf and lays it on the table: "Have the lama bless it." She also phones Dr. Hale tonight, so the doctor trusts you
more from your first meeting.
**· A lie** ("Just work, the study pays well."): she nods, does not believe you, and gives you nothing. Dr. Hale learns
the truth from her later, and trusts you a little less.**]**

In the corner, a man with a lined face is splicing an old rope with his hands, slowly. When he stands up, he limps.
**Uncle Pasang**, Pema's husband. He climbed with your parents' team twenty years ago and left four toes on the
mountain. He looks at you for a long time and says:

*"Come to the rock by the river at four. If you go up there, you clip like a Sherpa or you don't go at all."*

### Scene 3 · The stairs to the gompa (15:00, 6 minutes)

The gompa, the small monastery, sits above the village at the top of a long path of stone steps. The lama wants to
see you. You walk out into the cold sunlight, and for the first time the oximeter reacts while you move.

The steps are steep and uneven. Sangye's breathing gets louder with every step: you hear the air rushing in and out,
the heartbeat rising. On the screen the number slides: 84, 81, 78. Your stamina ring empties. Then, at the moment your
breath is loudest, one line appears at the bottom of the screen: *Hold to rest-step.* **[Taught at the exact moment
it is needed: finding 2. The village slope does the teaching: finding 3.]**

You hold it. Sangye's walk changes: a short step, a pause with the weight on the back leg, a breath, the next step. It
is slower, but the breath settles and the stamina ring stops emptying. If you try to sprint the last flight instead,
you can, for about ten steps, before the breath tears and the view swims. **[Develop and twist: the player chooses
and feels both.]**

At the top: prayer flags snapping in the wind, a row of brass prayer wheels along the wall. As you pass, you can spin
them, clockwise, one after another; each turns with a low rumble and a little bell. It does nothing to any number. It
just feels right. **[A ritual: belonging, not a mechanic.]**

Inside, butter lamps and the smell of juniper. The lama is very old and very calm. He does not ask why you are going.
He knows. He tells you, in a quiet voice, what the village has always said: that the mountain is the spine of the sky,
that something under it is withering, a great tree, and that something gnaws at its roots. That your parents did not
fall. *"They were taken. They became fuel for a fire that is going out."* And the old rule: *"Do not touch the spine of
the sky."*

**[If you told Pema the truth, you hold out the khata; he blesses it and puts it around your neck. You will wear it
in every camp.]**

He gives you a thin red blessing cord for your wrist and turns back to his lamps.

### Scene 4 · The compass on the roof (15:40, 3 minutes)

A ladder leads up to the gompa's flat roof. From here you see the whole valley: the river far below, the stone walls
of Dingboche, the trail running north towards Lobuche, and at the head of the valley the enormous wall of Lhotse and
Nuptse, with a long white banner of snow streaming from its crest. The summit of Everest hides behind that wall; you
cannot see it from here.

You take out the compass. *Open it.* The needle swings and settles on north. You turn on the spot and it holds north,
like any compass. **[Introduce, develop.]**

Then it moves. Slowly, away from north, the needle turns until it points at the wall, at the hidden summit, and stays
there. At the same moment there is a sharp heat in your right palm. You look down at your hand on screen: a thin golden
line, like the vein of a leaf, glows under the skin and fades. **[Twist: the first thing in the game that cannot be
explained.]**

It is gone in two seconds. A burn from the cold metal? Your heartbeat is up. The oximeter says 80%.

**[Design: the myth enters small, in daylight, and stays deniable (credible first). The compass is now on its key
for the rest of the game.]**

### Scene 5 · The practice rock (16:00, 9 minutes)

Down by the river there is a boulder the size of a house, its faces worn smooth, with old bolts and a faded fixed rope
on it. Pasang is waiting, sitting on a rock, his bad foot stretched out. Next to him lies a harness and a pair of thick
mitts.

*"Put it on."* The harness clicks around Sangye's waist. **[Clipping, in four steps: finding 5.]**

**Introduce.** A flat ledge along the bottom of the boulder, a rope along it, three anchors. At each anchor the word
*Clip* appears once: unclip from the rope behind you, clip onto the rope ahead, one press each, a satisfying double
click. You are never unclipped for more than a second. Pasang: "Good. Always one clip on the mountain. Always."

**Develop.** A slab of rock tilted at about 30 degrees, five anchors up its middle. Now there is no word on screen.
You clip at each anchor because you know how. **[No more prompts: the player has learned.]** Halfway up, the breathing
returns, and you use the rest-step without being told.

**Twist.** At the top of the slab, Pasang throws the mitts up to you. *"The cold is coming. Up there you clip with
these."* With the mitts on, each clip takes twice as long, and the hands on screen fumble. On the last anchor before
the top, if you rush, the carabiner misses the rope. You step up, unclipped, and your boot slips on the polished rock.

You slide. Half a metre, a metre, and then a hard jerk: the old rope Pasang has been holding the whole time, tied to
your harness without you noticing, stops you. He is laughing. *"Now you know why."* **[Twist with a safe failure: the
first mistake costs nothing but a scare. The same mistake on the Lhotse Face will cost everything.]**

**Conclude.** The final metres to the top of the boulder, where somebody has tied a string of prayer flags. You top
out. The sun is low now; the whole valley is turning gold and blue.

**[Optional, for skilled players (finding 6): a steep crack up the boulder's side instead of the slab. Harder, with
two awkward clips. At the top Pasang tosses you his own old carabiner, scratched and dented: "This was on the South
Col with your father." It hangs on your harness from now on and appears in your journal.]**

Pasang tells you, while you coil the rope, about the night he lost his toes: he was with your parents' team, and the
sirdar was a man called **Lhakpa**, the best climber in the Khumbu. *"Lhakpa's son works at Base Camp now. Nima. If
he is there, you climb behind him."* **[Foreshadowing the partner who joins in Chapter 3.]**

### Scene 6 · Packing (17:00, 5 minutes)

Back in the parents' house, by lamplight. Your pack lies open on the floor. *Open the pack* shows it: a list on the
left, the picture and description of each thing on the right, and at the top a thin bar for the weight, with a mark at
16 kg. **[The pack the author chose, introduced at home, in calm.]**

You already carry the basics: food, two thermoses, the rope and harness, the stove and gas, your mother's journal, the
compass. The bar sits at 13.8 kg. On the floor beside you are the extras you could take. **[Decision: choose three.]**

| Extra | Weight | What it does later |
|---|---|---|
| Two more rations | 1.2 kg | food when a storm pins you down |
| A third thermos | 1.0 kg | water when the gas runs low |
| The medicine kit | 0.3 kg | two doses that could save a life in Chapter 2, or later |
| Pasang's old down jacket | 1.1 kg | warmer at night |
| Spare mitts | 0.4 kg | save your fingers once |
| The photo from the wall | 0.1 kg | a little peace in every camp |

As you add things, the bar grows. Take too much and it passes the 16 kg mark and turns amber; walk across the room and
you feel Sangye slow down. **[The weight rule is felt, not read.]** You choose three and close the pack.

Dolma appears in the doorway with a crayon drawing she made this afternoon: three people on top of a triangle. None
of them is scribbled out. She puts it in your pack without asking. It weighs nothing.

### Scene 7 · Dusk: the peak of the chapter (18:15, 2 minutes)

Dolma takes your hand and pulls you up the ladder onto the flat roof of the house. *"Look!"*

The sun has gone from the village, but up at the head of the valley the snow banner streaming from the Lhotse wall
catches the last light: first pink, then a deep gold, burning above the dark rock. Everything below it is blue.

The compass, in your pocket, starts to hum. You can feel it more than hear it. You take it out: the needle points
straight at the glowing plume. The golden line in your palm lights up again, and this time it does not fade: it pulses,
slowly, in time with your heartbeat, which you can hear now, thud, thud, thud.

Very far away, inside the mountain, there is a sound like a single deep drum. Dolma looks up at you. *"Did you hear
that?"*

The gold leaves the plume. The mark fades. The needle swings back to north.

**[The peak of the chapter: finding 10. Wonder and dread together, earned by everything before it.]**

### Night · The end of the chapter (19:00, 2 minutes)

Dinner in the teahouse, the trekkers gone to bed, only family. Pasang says nothing; he puts a hand on your shoulder
once. Aunt Pema walks you to the door, touches her forehead to yours, the way your mother used to, and says only:
*"Come down."*

In the parents' house, Dolma has fallen asleep on the bench, still holding your hand. You do not let go. **[If you
release the hold, she murmurs and turns over. If you keep holding, the scene stays until you let go yourself.]**

A headlamp. Your mother's journal open on your knee, a pencil. On screen, the page **writes itself in Sangye's
handwriting** while you watch, built from what you actually did today:

*"Day 1. Dingboche. The house still smells of them. I found Mother's map: the line stops at 8,500 m, and she wrote
'Here'. Dolma held my hand all the way to the teahouse. Pasang made me clip with mitts on and I slipped, like a
child. On the roof the compass turned away from north and my hand burned. The lama says they were taken, not lost.
The oximeter says 84%. Tomorrow I leave. I don't know what I'm going to find. I only know which way the needle
points."*

**[The first page of the player's novel (Chapter_Design_v1, section 4.12). Its sentences change with the choices: the
truth or the lie, the extras, the crack or the slab, holding Dolma's hand or not.]**

The lamp goes out. In the dark, one line:

***The lama says they are not dead. Then what are they?***

**Chapter 1 · The Compass.** Then the alarm: 05:30, day 2. Chapter 2 begins.

---

## 6. What must be built

| Need | Today | Work |
|---|---|---|
| Prologue ridge at 8,500 m | the summit ridge exists (leg 7) | a scripted night start, a ghost figure (ghost material exists), regulator icing, a slip and axe prompt |
| Dingboche houses, teahouse, gompa, practice rock | the terrain exists; buildings do not | Fab Himalayan village kit; the parents' room dressed with props; positions measured on the real terrain [Open] |
| Aunt Pema, Dolma, Uncle Pasang, the lama | no NPCs exist | one character base with idle and talk animations; Dolma's hand-hold follow; Pasang's limp |
| Hold-a-hand input | not found | a follow-the-leader controller, later reused for the short-rope and the ending |
| Oximeter on the HUD from Scene 2 | the HUD shows oxygen today | reveal the HUD piece by piece (nothing on screen in Scene 1) |
| Rest-step | not found | Chapter_Design_v1, section 4.8 |
| Compass on the gompa roof, palm mark | compass exists | a scripted swing, the palm decal on the first-person hand |
| Practice rock with fixed line | fixed ropes exist (auto-clip) | manual clipping (Chapter_Design_v1, 4.8), mitts slow the clip, a hidden belay that catches the first slip |
| Pack with extras and weight bar | pack screen and weight exist | the extras and the "choose three" moment |
| Journal page that writes itself | not found | the template journal (Chapter_Design_v1, 4.12), first version |
| Dusk peak | the sky system exists | a timed sunset on the plume, the compass hum, a distant boom (the rumble effect exists) |

---

## 7. How we test it

Five to eight first-time players, each played alone while thinking aloud, then a short interview. The session monitor
records everything.

| Question | Measure | Target |
|---|---|---|
| Do they get control fast? | time from launch to first movement | under 30 seconds |
| Do they stay through the first ten minutes? | anyone who would stop before minute 10 (asked) | nobody |
| Can they learn without screens? | number of times they are stuck for more than 30 seconds | at most once each |
| Did the clip lesson land? | in Scene 5's develop step, clips made without a prompt | all of them |
| Did they find the compass without help? | (the Phase 1 test) | yes, every player |
| Do they remember the peak and the end? | "what moment do you remember most?" the next day | the roof at dusk, or the farewell |
| Is there an open question in their head? | "what do you want to know next?" | the woman on the ridge, or what the parents are |
| Do they care about someone? | "who would you miss if they were gone?" | Dolma, Pema or Pasang |

---

## 8. Sources

- Retention in the first minutes: [Google Play: why the first ten minutes are crucial](https://medium.com/googleplaydev/why-the-first-ten-minutes-is-crucial-if-you-want-to-keep-players-coming-back-to-your-mobile-game-4a89031b6308)
- Tutorials (Andersen et al., CHI 2012): [project page](https://grail.cs.washington.edu/projects/game-abtesting/chi2012/), [paper (PDF)](https://grail.cs.washington.edu/projects/game-abtesting/chi2012/chi2012.pdf)
- Teaching without words, World 1-1: [MCV: Miyamoto breaks down World 1-1](https://mcvuk.com/development-news/video-miyamoto-shares-his-level-design-secrets/), [Medium analysis](https://medium.com/swlh/the-perfect-game-tutorial-analyzing-super-marios-level-design-92f08c28bdf7), [Gamified UK](https://www.gamified.uk/2018/11/01/learning-from-games-onboarding-and-mario/)
- Four-step design (kishōtenketsu): [MCV: Nintendo's level design secrets in four steps](https://www.mcvuk.com/development/video-nintendos-level-design-secrets-in-four-steps), [Koichi Hayashida](https://en.wikipedia.org/wiki/Koichi_Hayashida), [Engadget](https://www.engadget.com/2015/03/17/super-mario-3d-world-design/)
- Cognitive load in onboarding: [GDevelop: text tutorials don't work](https://gdevelop.io/blog/improve-game-tutorials), [Wayline: the tutorial tax](https://www.wayline.io/blog/the-tutorial-tax-how-overly-long-tutorials-are-killing-your-game)
- Flow: [Jenova Chen, Flow in Games (abstract)](https://jenovachen.info/abstract), [Engadget on the thesis](https://www.engadget.com/2006-09-12-flow-in-games-an-interactive-thesis-on-dynamic-difficulty.html)
- In medias res openings: [Film Stories](https://filmstories.co.uk/features/in-media-res-beginning-in-the-middle/), [Giant Bomb](https://giantbomb.com/wiki/Concepts/In_Medias_Res), [Game Developer: how intros define titles](https://gamedeveloper.com/design/how-video-game-intros-define-titles)
- Environmental storytelling: [Game Developer: lessons from the theme park industry](https://www.gamedeveloper.com/design/environmental-storytelling-creating-immersive-3d-worlds-using-lessons-learned-from-the-theme-park-industry), [No Proscenium: narrative architectures](https://www.noproscenium.com/evocative-enacted-embedded-emergent-narrative-architectures-for-immersive-storytelling/)
- Ico and design by subtraction: [Ico](https://en.wikipedia.org/wiki/Ico), [Vice: how Ico inspired the companion](https://broadly.vice.com/en/article/jg8w5d/in-capturing-loneliness-ico-inspired-the-video-game-companion), [Game Developer: Ueda on his design ethos](https://gamedeveloper.com/design/-i-ico-i-creator-ueda-on-his-design-ethos-and-development-style)
- Peak-end rule: [Yu-kai Chou](https://yukaichou.com/behavioral-analysis/peak-end-rule-kahneman-experience-design/), [The Design Psychologist](https://designpsychologist.buzzsprout.com/2395044/episodes/17752274-the-peak-end-rule-in-design-what-we-take-away)
- Open loops: [Psychology of Games: the Zeigarnik effect](https://www.psychologyofgames.com/2013/03/the-zeigarnik-effect-and-quest-logs)
- Self-determination theory and games: [Ryan, Rigby, Przybylski 2006 (PDF)](https://www.rochester.edu/warner/lida/wp-content/uploads/2022/11/02bfe513dd59366750000000.pdf)

Strength of the evidence: the Andersen study and the self-determination papers are peer-reviewed; the Google Play
analysis is industry data; the Nintendo, Ico, flow and storytelling material is designers' practice from interviews and
analyses; the peak-end rule is well established in psychology but only applied to games by practitioners.
