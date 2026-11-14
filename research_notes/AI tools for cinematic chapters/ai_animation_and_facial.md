# AI tools for character body and facial animation without a mocap studio (UE 5.8, solo developer, Steam release)

Research date: 2026-11-14. Many vendor pricing pages are JavaScript-rendered and could not be read directly; where a figure comes from a third-party tracker or an older article this is stated. Treat every price as "check on the day of purchase".

## Video-to-mocap: which tools, what they cost, licence, hands, cameras, export, UE path, scriptability

### Takeaway
The biggest 2026 change is that Epic shipped a **free, commercial-use-allowed, local, Windows-only MetaHuman Animator Markerless Motion Capture plugin for UE 5.8** (body + face + fingers from one webcam or phone video, built on tech from Meshcapade, which Epic bought in February 2026). It is scriptable through Python/Blueprints inside the editor, so it is the default first choice for this project. Paid cloud services (Move One, Rokoko Vision inside Rokoko Create, DeepMotion, Plask, QuickMagic, Flow Studio, Uthana video-to-motion) remain useful as fallbacks; their free tiers are often non-commercial or tiny. FreeMoCap is AGPL software (the software licence, not your captured data, is the concern).

### Cited Findings

**MetaHuman Animator Markerless Motion Capture plugin (Epic, free)**
- Released alongside UE 5.8 (mid-June 2026). Extracts full-body animation *including finger motion* from a single mono camera (webcam or phone) and applies it to a MetaHuman; can solve face and body from the same clip; dual-camera mode lets the face come from a separate head-mounted capture — [CG Channel, 18 Jun 2026](https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/)
- Licence/price: "It's free, including for use on commercial projects." Processing is local, not cloud. Compatible with "Unreal Engine 5.8 running on Windows only" — [CG Channel](https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/)
- Based on tech from Meshcapade, which Epic acquired earlier in 2026 — [CG Channel](https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/)
- Early user test (Bryan Steagall): better than expected, especially fingers (solved at distance and when occluded); "some floating feet which is very much expected"; weaker with moving cameras (use a static camera). Social-media reports: a second of 1080p footage processed in under a minute — [CG Channel](https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/)
- Marked Experimental; ships as a standalone plugin via Fab; body animation features are Windows-only — [MetaHuman 5.8 release notes](https://dev.epicgames.com/documentation/metahuman/metahuman-5-8-release-notes-in-unreal-engine?lang=en-US)
- Workflow: create a MetaHuman Performance asset, assign the capture, turn facial off/body tracking on, pick any MetaHuman target, Process. Automation is available through Python and Blueprints — [Epic docs: MetaHuman Animation from Mono Video](https://dev.epicgames.com/documentation/metahuman/metahuman-animation-from-mono-video-capture-in-unreal-engine) (via search summary); MetaHuman Animator also "offers an improved API for batch processing large volumes of captured performance data" — [MetaHuman 5.8 release notes](https://dev.epicgames.com/documentation/metahuman/metahuman-5-8-release-notes-in-unreal-engine?lang=en-US)
- Michael Black (Meshcapade/MPI) says body pose is 9% more accurate than the prior version (vendor's own testing) — [X post](https://x.com/Michael_J_Black/status/2067342601511141875)
- Open licensing question: a solo dev (Steam game, UE 5.8.2) asked in Oct 2026 whether the plugin's *runtime models* (CHMR, HUE, Detectron2, SMPL-X data) may be shipped inside a packaged game; no Epic answer as of the thread; a community reply advises editor-only use until Epic answers in writing — [UE forum, 12–13 Oct 2026](https://forums.unrealengine.com/t/licensing-question-shipping-metahuman-animator-markerless-motion-capture-runtime-models-in-a-steam-game/2839692)

**Meshcapade (MoCapade, MoGen) — now Epic**
- Epic announced the acquisition on 18 Feb 2026; team joins Epic's AI Research team for UE and MetaHuman — [80.lv, 24 Feb 2026](https://80.lv/articles/ai-motion-capture-startup-meshcapade-now-part-of-epic-games); [heise, 19 Feb 2026](https://heise.de/-11182302)
- One secondary source reports Meshcapade's online platforms (me.meshcapade.com, editor, API) closed on 18 Apr 2026 with no public replacement for the consumer tool — reported in search summary of [MediaPost](https://www.mediapost.com/publications/article/412940/epic-games-acquires-firm-that-makes-3d-humans.html)/aggregators; **not confirmed by Epic or Meshcapade** in anything I could read.

**Move.ai — Move One (single iPhone) and Move Pro (multi-camera)**
- Move One official pricing: Free = 30 one-time credits, 30-second max videos; Starter $18/mo (60 credits/mo); Standard $48/mo (180); Plus $225/mo (240); Advanced $490/mo (700); +VAT; paid plans allow 60-second videos. Gen 1 = 1 credit per person per second; Gen 2 = 2 credits per person per second. Credits do not roll over; purchased credits need an active subscription. Page describes Move One only as an iPhone app; no date shown (copyright 2026) — [Move docs: Move One Pricing](https://docs.move.ai/knowledge/move-one-pricing)
- So Starter = 60 s of Gen 1 or 30 s of Gen 2 per month (calculated from the page above).
- Licence tiers: Move docs say non-commercial use covers personal projects, education, research; aggregators list a $15/mo Personal plan as non-commercial and Standard/Advanced as commercial — [Move docs pricing (search snippet)](https://docs.move.ai/knowledge/move-ai-pricing-plans-credits); [aitoolsatlas (verified ~Mar 2026)](https://aitoolsatlas.ai/tools/move-ai/pricing). **Conflict**: the official Move One page tier names/prices ($18/$48/$225/$490) do not match aggregator tiers ($15/$50/$250); the commercial status of the $18 Starter tier is unconfirmed.
- Hand/finger tracking ("Dex") available for Move One and Move Pro Gen 2 models; Move Pro trial = $995 for 1 month, 1,000 credits, up to 4 cameras, 1 person, finger tracking; full Move Pro pricing is sales-only; enterprise tier carries an "Enterprise commercial license" — [Move docs: Move Pro 2.0 self-serve (search snippet)](https://docs.move.ai/knowledge/move-pro-2.0-self-serve); [move.ai/dex](https://www.move.ai/dex)
- iPhone required for Move One (Android not mentioned); Windows irrelevant (cloud processing).

**Rokoko Vision (now inside Rokoko Create)**
- As of 16 Jul 2026 Rokoko Vision became part of Rokoko Create; the Vision site redirects to Create; Vision 3.0 = monocular solver "rebuilt from the ground up"; the dual-camera workflow was retired ~16 Aug 2026; all Rokoko AI mocap now happens via video upload inside Create — [Rokoko support: Vision and Create](https://support.rokoko.com/hc/en-us/articles/48823033216017-Rokoko-Vision-and-Rokoko-Create-What-you-need-to-know) (search summary; page returned 403 to direct fetch); [Rokoko: Introducing Vision 3.0](https://www.rokoko.com/insights/introducing-rokoko-vision-3-0)
- Rokoko Studio plans (pricing page, detailed table): Starter $0 = 30 s/month Vision processing, 5 text-to-motion imports/month; Basic $10/m annual ($12 monthly) = 600 s/month, 100 imports; Plus $20/m annual ($28 monthly) = 3,000 s/month, 1,000 imports; Pro $50/m annual ($70 monthly) = 15,000 s/month, unlimited imports; Enterprise from $100/m. FBX on all plans; BVH from Basic (FAQ says Plus/Pro); CSV on Pro. **Conflict**: the same page's summary cards show Basic $20/m and Plus $50/m — [Rokoko pricing](https://rokoko.com/pricing/)
- Commercial use: the page says Motion Library animations can be used commercially; it does not state commercial terms per plan — [Rokoko pricing](https://rokoko.com/pricing/)
- Finger capture from video is weak; review describes Rokoko webcam finger tracking as "basic" and says fingers need Smartgloves — [StraySpark, Mar 2026 (vendor blog)](https://www.strayspark.studio/blog/ai-mocap-indie-developers-2026-comparison)
- Studio runs on Windows 10+ and macOS — [CG Channel, 3 Apr 2026](https://www.cgchannel.com/2026/04/rokoko-create-generates-full-body-animations-for-free/)

**DeepMotion Animate 3D**
- Freemium: up to 60 s of animation per month, "available for personal, non-commercial use"; commercial licensing requires a premium subscription. 1 credit = 1 s of animation; face tracking and hand tracking each cost +0.5 credit/s. Exports FBX, BVH, MP4, GLB (GLB only for custom characters). API plans for high volume via contact — [DeepMotion Animate 3D pricing page (FAQ text)](https://www.deepmotion.com/pricing-animate3d)
- Dollar prices could not be read from the official page; third-party directories list Animate 3D at Free / $15 / $48 / $199 per month (unverified, undated) — [Toolmage](https://www.toolmage.com/en/tool/deepmotion/); a Viggle blog notes DeepMotion publishes no dollar figures — [Viggle](https://viggle.ai/blog/best-ai-animation-software-3d-character-work)

**Plask Motion**
- Free (no subscription): 15 s of mocap daily, full body + hand capture, multi-person, FBX/GLB/BVH; Standard $18/mo billed annually = 10 min/month; Pro $50/mo billed annually = 1 h/month. Commercial rights per plan are not stated on the pricing page — [Plask pricing](https://plask.ai/pricing)

**QuickMagic**
- Pricing from aggregators only: ~$9.90/mo starter, $49.90/mo professional, credit-based; claims full body, half body, face and hand capture — [TopAI.tools](https://topai.tools/t/quickmagic-motion-capture); [Dataconomy](https://dataconomy.com/tools/quickmagic/)
- QuickMagic powers Reallusion iClone's Video Mocap, sold at $2.50 per clip — [Digital Production, 3 Nov 2025](https://digitalproduction.com/2025/11/03/video-mocap-reallusions-ai-gets-moving/)

**Autodesk Flow Studio (formerly Wonder Studio)**
- Exports FBX or USD for Maya/Blender/Unreal/3ds Max; body and hand tracking exported as FBX; face tracking exported as a Blender scene — [Autodesk Flow Studio product details (search snippet)](https://www.autodesk.com/ca-en/products/flow-studio/product-details)
- Free tier watermarks output ("essentially a trial edition") with 300 credits/month ≈ 30 s AI mocap; Standard $45/mo or $360/yr with 6,000 credits/month (Aug 2025) — [CG Channel, 12 Aug 2025](https://www.cgchannel.com/2025/08/autodesk-cuts-prices-of-flow-studio-subscriptions/); tracker lists Free / Lite $10 / Standard $45 / Pro $95 — [usagepricing.com](https://www.usagepricing.com/blueprint/wonder-dynamics) (Autodesk page blocked direct fetch, 403)

**Radical (RADiCAL)**
- No shutdown news found; May 2026 snapshot says the pricing page is sales-led with no self-serve tiers — [xpay.sh](https://www.xpay.sh/saas-pricing/radicalmotion/); other directories still claim a freemium tier — [TheresAnAIForThat](https://theresanaiforthat.com/ai/radicalmotion/). Status unclear.

**Uthana (video-to-motion + text-to-motion, API-first)**
- Pay-as-you-go credits; new subscriptions stopped Aug 2026. Video-to-motion v2.0/2.1 $0.05/s, v3.0 $0.06/s; auto-rigging $0.15/rig. GraphQL API uses the same credit balance. "Paid usage includes commercial use of generated outputs, subject to Uthana's Terms of Use" — [Uthana pricing](https://uthana.com/pricing)

**FreeMoCap (open source)**
- Licence AGPLv3; maintainers will negotiate separate commercial terms "at a price point that rises the further you move from the AGPL's terms" — [PyPI freemocap](https://pypi.org/project/freemocap/); [awesome.ecosyste.ms](https://awesome.ecosyste.ms/projects/github.com%2Ffreemocap%2Ffreemocap)
- Multi-webcam markerless system (research-grade claim) — [Menon Lab blog](https://themenonlab.blog/blog/freemocap-open-source-motion-capture)

**Common quality problems in video mocap**
- Foot sliding, depth errors and poor hand tracking are the typical failures; Move.ai pipeline guides report occasional foot sliding and drift on fast moves; Rokoko Studio added live foot-lock to reduce sliding — [StraySpark Move.ai pipeline](https://www.strayspark.studio/blog/moveai-mocap-without-suits-indie-animation-pipeline); [Rokoko support: Real-time Locomotion & Drift Fix](https://support.rokoko.com/hc/en-us/articles/41620261484945-Rokoko-Studio-Update-Real-time-Locomotion-Drift-Fix)
- Cascadeur 2025.2 added automatic global-position improvement and foot-slide cleaning for video mocap — [CG Channel tag page / Cascadeur 2025.2](https://cascadeur.com/blog/view/cascadeur-2025-2-brings-massive-ai-inbetweening-workflow-upgrades)

**Getting results onto a UE 5.8 skeleton (applies to all tools)**
- Python batch retarget: `unreal.IKRetargetBatchOperation.run_batch_retarget` duplicates animations and retargets them with a supplied IK Retargeter — [Epic Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/IKRetargetBatchOperation); IK Retargeter assets can be created/edited from Python — [Epic docs](https://dev.epicgames.com/documentation/unreal-engine/using-python-to-create-and-edit-ik-retargeter-assets-in-unreal-engine)
- "Auto Generate Retargeter" in the Retarget Animations window creates IK Rigs + Retargeter automatically — [Epic: Auto Retargeting](https://dev.epicgames.com/documentation/unreal-engine/auto-retargeting-in-unreal-engine?lang=ja) (no documented Python equivalent found)
- Third-party "Mixamo Animation Retargeting 2" plugin auto-creates IK Rig/Retargeter assets for Mixamo skeletons; 5.6 support added for the new retarget op stack — [Fab listing](https://www.fab.com/listings/5eaf2746-1891-435a-837c-67c9f7a847f4?lang=en)
- One user reported 5.6 retargeting to MetaHuman gave very different results from 5.5 — [UE forum](https://forums.unrealengine.com/t/5-6-animation-retargeting-bug/2521595)

### Inferences
- For this project the markerless plugin's output lands on a MetaHuman body skeleton; because the MetaHuman body is UE5-mannequin-family, retargeting to the existing Mixamo-rigged NPCs and the AI-rigged climber is a standard IK Retargeter hop (the project already has a Climber_Rigged retarget). Cutscene animation is baked into AnimSequences, so the open question about shipping the plugin's runtime models does not apply if solving stays in the editor.
- Because the plugin runs inside the editor and exposes Python/Blueprint automation, Claude Code can drive it via the project's existing editor Python remote-exec path (record video -> drop into folder -> script creates Performance assets, processes, exports AnimSequence, batch-retargets). No cloud service offers that level of agent control for $0.
- Single-camera video tools all share floating/sliding feet; budget a foot-lock/IK pass (Control Rig in UE, or Cascadeur AutoPhysics) for every clip.
- Uthana is the most agent-friendly paid fallback (documented GraphQL API, per-second pricing, commercial use on paid usage): 1 minute of v3 video-to-motion = 60 x $0.06 = $3.60.
- Move One requires an iPhone; the Epic plugin, Rokoko Create, DeepMotion, Plask, QuickMagic, Flow Studio and Uthana accept uploaded video from any camera.

### Gaps
- Could not confirm Move One's commercial licence for the $18 Starter tier, or whether Move has a public API in 2026.
- No official 2026 dollar prices for DeepMotion, QuickMagic, Radical, Flow Studio (pages blocked/JS-only).
- Plask's and Rokoko's per-plan commercial terms are not on their pricing pages (Terms of Service not read).
- Meshcapade platform shutdown (18 Apr 2026) is from secondary sources only.
- Multi-person support in the Epic markerless plugin was not documented in what I read (release notes say "single actor").
- No independent side-by-side quality test of these tools on the same footage was found.

## Text-to-motion and generative motion: which are usable commercially?

### Takeaway
Commercially safe options exist: **Uthana** (paid per second, commercial use included, API), **DeepMotion SayMotion** (paid plans only), **Rokoko Create** (free generation, export via Rokoko Studio, terms per plan unstated), **NVIDIA Kimodo** SOMA/G1 checkpoints (NVIDIA Open Model License, commercial with attribution; local, scriptable), and **Cascadeur** (AI inbetweening/AutoPosing/AutoPhysics; Indie $96/yr under $100k revenue). **Avoid for a Steam game**: Kimodo-SMPLX (R&D licence), and pretrained MoMask/MDM/MotionGPT-style research weights trained on HumanML3D/AMASS (AMASS licence forbids commercial use and commercial training). Tencent HY-Motion 1.0 has a community licence that excludes the EU, UK and South Korea and needs 24–26 GB VRAM.

### Cited Findings

**NVIDIA Kimodo (released ~Mar–Apr 2026)**
- Kinematic motion diffusion trained on ~700 h optical mocap; controlled by text prompts on a timeline plus constraints: full-body keyframes, 2D root waypoints/paths, hand/foot end-effector positions and rotations — [Kimodo README](https://raw.githubusercontent.com/nv-tlabs/kimodo/main/README.md); [project page](https://research.nvidia.com/labs/sil/projects/kimodo/)
- Checkpoint licences: all SOMA and Unitree G1 checkpoints (RP v1/v1.1/v1.2, SEED v1/v1.1) = NVIDIA Open Model License; **Kimodo-SMPLX-RP-v1 = NVIDIA R&D Model license** (non-commercial) — [Kimodo README](https://raw.githubusercontent.com/nv-tlabs/kimodo/main/README.md); [HF card SMPLX](https://huggingface.co/nvidia/Kimodo-SMPLX-RP-v1)
- Code is Apache-2.0; the BVH export module carries an Apache-2.0 header — [Kimodo docs: kimodo.exports.bvh](https://research.nvidia.com/labs/sil/projects/kimodo/docs/_modules/kimodo/exports/bvh.html); NVIDIA Open Model License permits commercial use with attribution — [HF model card mirror](https://huggingface.co/vigneshk0702/Kimodo-SOMA-RP-v1) (search summary)
- Output: default NPZ (joint positions, rotations, foot contacts, root); MuJoCo CSV (G1); AMASS NPZ (SMPL-X); BVH via the export module. ~17 GB VRAM all-on-GPU, under 3 GB with `TEXT_ENCODER_DEVICE=cpu`; tested on RTX 3090/4090/A100; developed on Linux, "Windows should work especially if using Docker". No Blender/Unreal/Maya integration in the README — [Kimodo README](https://raw.githubusercontent.com/nv-tlabs/kimodo/main/README.md)
- A third-party "Kimodo Blender Bridge" add-on exists — [Superhive market](https://superhivemarket.com/products/kimodoblenderbridge/docs) (not evaluated)

**Tencent HY-Motion 1.0 (Dec 2025)**
- 1.0B-param model needs 26 GB VRAM minimum; Lite (0.46B) 24 GB; output described as "skeleton-based 3D character animations" with no stated file format — [GitHub HY-Motion-1.0](https://github.com/Tencent-Hunyuan/HY-Motion-1.0)
- Licence "tencent-hunyuan-community" — [Hugging Face card](https://huggingface.co/tencent/HY-Motion-1.0); a GitHub issue quotes the licence as not applying in the EU, UK and South Korea (territory excludes them) — [GitHub issue #49](https://github.com/Tencent-Hunyuan/HY-Motion-1.0/issues/49). Full licence text could not be fetched (404 on raw file).

**MoMask / MDM / MotionGPT (research)**
- MoMask code is MIT but "depends on other libraries, including SMPL, SMPL-X, PyTorch3D, and uses datasets which each have their own respective licenses that must also be followed"; outputs BVH; authors found Rokoko retargeting "usually leads to large error on foot" and recommend keemap.rig.transfer in Blender — [MoMask README](https://github.com/EricGuo5513/momask-codes)
- HumanML3D pairs text with 14,616 motion sequences from AMASS — [MoMask paper/aggregator](https://www.roboticscenter.ai/datasets/humanml3d/); MotionGPT trains on part of AMASS — [MotionGPT paper](https://arxiv.org/pdf/2306.14795)
- AMASS licence: non-commercial only; prohibits "incorporation in a commercial product" and training "methods/algorithms/neural networks/etc. for commercial use of any kind"; commercial licensing via ps-license@tue.mpg.de — [AMASS licence](https://amass.is.tue.mpg.de/license.html)

**Uthana text-to-motion**
- Text-to-motion v1.0 $0.02/s, v2.0 $0.03/s, v3.0 $0.10/s; locomotion $0.02/s; stitching/looping $0.01/s; library search $0.10/s; character generation $0.75; GraphQL API; paid usage includes commercial use — [Uthana pricing](https://uthana.com/pricing); Unreal listed among supported workflows — [Uthana (search snippet)](https://uthana.com/llms.txt)

**DeepMotion SayMotion**
- Official page: free tier 5 animation credits/month and 1 download/month; Commercial License is a paid-plan feature; separate API page — [SayMotion pricing (search snippet)](https://deepmotion.com/pricing-saymotion); SayMotion V2.3 post: "users must now upgrade to one of our subscription plans for a Commercial License" — [DeepMotion blog](https://deepmotion.com/post/saymotion-v2-3-video-text-to-3d-animation-in-one-platform-animate-3d-integration)
- 2024 prices (likely outdated): $15/mo or $108/yr for 50 credits; $300/mo or $996/yr for 1,000 credits; 1 credit = 10 s generation or one edit — [CG Channel, Jun/Oct 2024](https://www.cgchannel.com/?p=160406)

**Rokoko Create (text-to-motion, Apr 2026)**
- Browser tool generating simple full-body motions (idles, walks, runs) from prompts; trained on Rokoko's proprietary mocap library; free generation; export requires Rokoko Studio (Starter: 5 text-to-motion FBX exports/month; paid from $120/yr); positioned for prototyping/blocking and "small animation gaps", not final complex animation; foot slippage cleaned in Studio; licensing not addressed in the article — [CG Channel, 3 Apr 2026](https://www.cgchannel.com/2026/04/rokoko-create-generates-full-body-animations-for-free/)

**Cascadeur (AI-assisted keyframing)**
- Plans (USD, FAQ): Free $0 (non-commercial; export .casc only, no FBX/DAE; 300 frames, 120 joints); Indie $96/yr or $19/mo (revenue < $100k/yr); Pro $396/yr or $49/mo (unlimited); Teams $396/yr per seat; Educational $49/yr (no commercial). Yearly Indie/Pro give perpetual-licence builds — [Cascadeur plans](https://cascadeur.com/plans)
- Possible Nov 2026 change: a tracker says annual billing options were removed on 4 Nov 2026 — [Toolradar](https://toolradar.com/tools/cascadeur/pricing) (unconfirmed)
- AI Inbetweening (2025.1, up to 89 frames; 120 frames + motion-style presets walk/run/crawl/jump/fall/acrobatic/combat in 2025.2; became an interpolation type in 2025.3); AutoPosing incl. fingers; AutoPhysics; 2026.1 added Root Motion system and a new UE5 Live Link; 2026.2 added animation layers and BVH import — [Cascadeur 2025.2 blog](https://cascadeur.com/blog/view/cascadeur-2025-2-brings-massive-ai-inbetweening-workflow-upgrades); [CG Channel 2025.3](https://www.cgchannel.com/2025/11/nekki-releases-cascadeur-2025-3/); [CG Channel tag pages 2026.1/2026.2](https://www.cgchannel.com/tag/ai-inbetweening/)

**NVIDIA ACE / Omniverse text-to-motion**
- NVIDIA's ACE for Games page lists no text-to-motion or body-animation models; its animation items are audio-to-face only — [NVIDIA ACE for Games](https://developer.nvidia.com/ace-for-games)

**Epic in-engine options**
- Game Animation Sample: originally "over 500 AAA-quality animations" with a working character and motion-matching AnimBP (Jun 2024) — [Epic blog](https://www.unrealengine.com/blog/game-animation-sample?lang=en-US); updated for UE 5.8 on 12 Aug 2026 (Chooser can hold multiple Pose Search Databases) — [Epic tech blog](https://www.unrealengine.com/tech-blog/download-the-latest-game-animation-sample-project-now-updated-for-ue-5-8)
- UE 5.8 added in-editor rigging/animation tools (Direct Mesh Controls, sculpt-driven facial workflows, shot sculpting) and an MCP plugin that lets LLMs connect to the engine — [UE 5.8 coverage, gamefromscratch](https://gamefromscratch.com/unreal-engine-5-8-released/); [CG Channel 5 key features](https://www.cgchannel.com/2026/06/see-5-key-features-for-cg-artists-in-unreal-engine-5-8/)

### Inferences
- Kimodo is the best agent-drivable, local, commercially usable generator for this project: Claude Code can script a CLI/Python call with a text prompt plus a root path (e.g., "Sherpa walks along this rope line, stops, turns to player"), export BVH, import to UE and batch-retarget. Needs one-time IK Rig for the SOMA skeleton. Running the text encoder on CPU keeps VRAM under 3 GB, which suits a laptop GPU (speed not measured). Must ship NVIDIA attribution.
- HY-Motion is impractical on a laptop (24–26 GB VRAM) and its territorial clause must be read in full before any use.
- Treat all HumanML3D/AMASS-trained research checkpoints as **not Steam-safe**, regardless of the MIT/Apache code licence.
- Text-to-motion today is good for idles, walks, gestures, reaction beats; acting with a specific partner (child holding the player's hand, handing over an oxygen bottle) still needs video capture or keyframing with Cascadeur/UE Control Rig.

### Gaps
- MDM and MotionGPT repository licence files were not read.
- Nvidia "text-to-motion in Omniverse" product: found nothing beyond Kimodo.
- Uthana's Terms of Use (output ownership, attribution, AI-training rights) not read; Unreal skeleton presets not confirmed.
- Rokoko Create output licence not stated.
- Cascadeur Python scripting API not verified in this pass.
- The "Mover" plugin and ML Deformer were not researched (they are movement/deformation systems, not animation generators).

## Free animation libraries usable commercially

### Takeaway
Mixamo (royalty-free, no attribution, no raw-file resale) and Epic's Game Animation Sample (UE-only content licence, updated for 5.8) are the safe free bases. CMU mocap is reported as free for commercial use. ActorCore free motions may need a separate free "Mass Distribution License" for games. Read each Fab item's licence.

### Cited Findings
- Mixamo: characters and animations royalty-free in commercial projects including games; not allowed: reselling Mixamo assets, bulk-downloading, or asset packs that redistribute raw files; attribution not required — [Adobe community Mixamo FAQ](https://community.adobe.com/questions-696/mixamo-faq-licensing-royalties-ownership-eula-and-tos-589400); terms are those in force at time of use — [Adobe community](https://community.adobe.com/questions-696/where-can-i-find-legal-documentation-related-to-the-use-of-mixamo-675146)
- Game Animation Sample licence heading reported as "UE-Only Content - Licensed for Use Only with Unreal Engine-based Products" (forum paraphrase) — [UE forum](https://forums.unrealengine.com/t/what-can-i-do-with-game-animation-sample-that-recently-was-at-100-off/1903980); listed free; updated for 5.8 — [Epic tech blog](https://www.unrealengine.com/tech-blog/download-the-latest-game-animation-sample-project-now-updated-for-ue-5-8)
- Fab assets can be used in a packaged commercial game but not resold — [UE forum](https://forums.unrealengine.com/t/using-free-fab-ue-marketplace-skeletal-meshes-and-animations-in-a-commercial-project/2259858)
- ActorCore: free assets reported as commercially licensed (2022), but a Reallusion announcement says games/apps require requesting a free Mass Distribution License — [CG Channel 2022](https://www.cgchannel.com/?p=121885); [Reallusion forum](https://forum.reallusion.com/PrintTopic493214.aspx)
- CMU Graphics Lab Motion Capture Database: "free for use in research and commercial projects worldwide" (quoted by secondary sites) — [roboticscenter.ai](https://www.roboticscenter.ai/datasets/cmu-mocap/); [cgpress](https://cgpress.org/archives/carnegie-mellon-university-motion-capture-database.html). Direct fetch of mocap.cs.cmu.edu failed (certificate error).
- Rokoko Motion Library animations may be used in commercial projects — [Rokoko pricing](https://rokoko.com/pricing/)

### Inferences
- For 17 chapters, a free base (GASP locomotion + Mixamo gestures) covers walking/idle/sitting; AI tools are best spent on the bespoke acting beats.

### Gaps
- Current animation count in the 5.8 Game Animation Sample not found.
- Fab "free monthly" animation packs and MoCap Online free packs were not researched.
- Exact current ActorCore EULA and CMU wording not read from primary pages.

## Facial animation and lip-sync inside UE 5.8

### Takeaway
**MetaHuman Animator audio-driven animation** (UE 5.6+, improved real-time model in 5.8) is free, in-engine, offline or real-time, with mood, head motion and blinks, and supports batch processing; no iPhone is needed (webcam/mono video also supported for facial capture). It targets MetaHuman faces; non-MetaHuman characters need curve-to-morph mapping (ARKit 52 shapes help). **NVIDIA Audio2Face-3D** is open (plugin MIT, models NVIDIA Open Model License) but the official UE plugin lists only up to UE 5.6. **OVRLipSync is end-of-life.** Paid middleware (FaceFX, Speech Graphics SGX) has no public pricing.

### Cited Findings

**MetaHuman Animator — audio-driven**
- Processes audio (any UE-supported SoundWave) into facial animation, real-time or offline; offline solve can add head motion and a mood (Auto Detect, Happy, Sad, Anger…) with intensity; blinks optional; a process mask can output mouth-only curves for layering; outputs facial rig tracks in Sequencer, exportable as Animation Sequence or Level Sequence; realtime solver produces no head motion; requires UE 5.6+ and the MetaHuman Animator plugin; batch processing via the Content Browser — [Epic docs: Audio Driven Animation](https://dev.epicgames.com/documentation/en-us/metahuman/audio-driven-animation)
- 5.8: "Audio-driven animation benefits from a new real-time model"; MetaHuman Animator facial solving now runs on Linux and macOS (body features remain Windows-only); Live Link MetaHuman Animator accepts streamed RTSP video for real-time facial solving; webcams and supported smartphones work — [MetaHuman 5.8 release notes](https://dev.epicgames.com/documentation/metahuman/metahuman-5-8-release-notes-in-unreal-engine?lang=en-US)
- 5.6 made MetaHuman Animator work from almost any mono camera (webcam, most smartphones) or audio — [PC Gamer](https://www.pcgamer.com/hardware/unreals-metahuman-animator-can-generate-surprisingly-lifelike-animation-with-just-your-phone-and-its-now-integrated-directly-into-unreal-engine-5-6); [UE 5.6 release](https://gamefromscratch.com/unreal-engine-5-6-released/)
- Non-MetaHuman use: Unreal Fest 2025 talk shows exporting the animation as a sequence and mapping morph targets to a non-MetaHuman character's facial controls — [UE forum talk page](https://forums.unrealengine.com/t/talks-and-demos-using-the-metahuman-ecosystem-for-non-metahuman-animation-unreal-fest-orlando-2025/2674022); a user with ARKit-52-shape characters reports success with MetaHuman audio-driven animation (anecdotal) — [UE forum](https://forums.unrealengine.com/t/audio-driven-animation-retarget/2745931)

**NVIDIA Audio2Face-3D**
- Open-sourced late Sep 2025 — [NVIDIA blog](https://developer.nvidia.com/blog/nvidia-open-sources-audio2face-animation-model); [PC Gamer](https://www.pcgamer.com/hardware/graphics-cards/nvidias-gen-ai-facial-animation-tech-is-going-open-source-potentially-making-it-much-easier-for-game-devs-to-turn-real-time-speech-into-convincing-characters)
- UE plugins "governed by the MIT license"; SDK (C++/Python) MIT; Audio2Face-3D models open weights under NVIDIA Open Model License; Audio2Emotion models custom licence; training framework Apache, sample data evaluation-only; plugin downloads: ACE Reference Plugin v2.5.0rc3 for UE 5.5/5.6, v2.4.0 for 5.4 — **no 5.7/5.8 build listed for Audio2Face**; described as local, RTX-optimised; offline Maya ACE plugin also listed — [NVIDIA ACE for Games](https://developer.nvidia.com/ace-for-games)
- NVIDIA ACE docs have a "Notes for non-MetaHuman characters" section — [ACE Unreal plugin 2.5 docs](https://docs.nvidia.com/ace/ace-unreal-plugin/2.5/ace-unreal-plugin-animation.html)

**Meta OVRLipSync**
- Meta docs (updated 17 Apr 2026) say the Oculus Lipsync plugin is in end-of-life stage with no further updates; successor is audio-to-viseme in the Movement SDK via OpenXR — [Meta developer docs](https://developers.meta.com/horizon/documentation/unreal/audio-ovrlipsync-unreal/); an unofficial UE5 port exists — [UE forum](https://forums.unrealengine.com/t/lipsync-plugins/1910572); Convai deprecated its OVRLipSync component in favour of FaceSync (2024) — [Convai docs](https://docs.convai.com/api-docs/plugins-and-integrations/unreal-engine/guides/creating-metahuman-characters/adding-lipsync-to-metahuman)

**Fab plugins**
- Runtime MetaHuman Lip Sync (Georgy Dev): fully offline, cross-platform; MetaHuman plus custom characters (Daz Genesis 8/9, CC3/CC4, Mixamo, FACS, ARKit, custom morph targets); Mood-Enabled and Realistic models are MetaHuman-only — [Fab listing](https://www.fab.com/listings/028ebd26-2310-4c93-9094-cafe2c03f6a5); [plugin docs](https://georgydev-docs.pages.dev/runtime-metahuman-lip-sync/overview); [UE forum thread](https://forums.unrealengine.com/t/runtime-metahuman-lip-sync-real-time-offline-lip-sync-with-audio-tts-ai-for-npc-cc4-genesis-arkit-and-more-facial-animations/2383289). Price not found.

**Commercial middleware**
- FaceFX UE5 plugin on GitHub supports UE 5.6; no current price found (old UE4 runtime was free under $500k revenue, historical) — [FaceFX-UE5 GitHub](https://github.com/FaceFX/FaceFX-UE5); [AppDeveloper Magazine 2015](https://appdevelopermagazine.com/facefx-releases-plugin-for-unreal-engine-4/)
- Speech Graphics SGX: "various packages"; no public prices — [Speech Graphics FAQ](https://mayflower-puma-nl6b.squarespace.com/faqs)

### Inferences
- Lowest-cost path for 17 chapters of dialogue: record or generate voice lines -> MetaHuman Animator audio-driven offline solve (batch) -> bake to AnimSequence. If NPCs stay Mixamo/CC-style, either give them ARKit-52 blendshapes and map MetaHuman curves, or use Runtime MetaHuman Lip Sync's custom-character path. Both are scriptable from editor Python (batch processing documented for MHA).
- Audio2Face would need the developer (or Claude Code) to rebuild the MIT plugin source for 5.8; that is possible but is extra maintenance, and it is tied to RTX GPUs.
- For a first-person game where faces are seen briefly and often in goggles, balaclavas and hoods, audio-driven solve quality is likely enough; webcam facial capture is a step up for hero close-ups.

### Gaps
- JALI, Inworld and Convai current lip-sync products/pricing were not researched in depth.
- Wav2Lip-style tools: licence not checked in this pass (my prior knowledge is that the original Wav2Lip weights are non-commercial — unverified here); they produce 2D video, not rig curves, so they do not fit a UE pipeline.
- Whether MetaHuman Animator's audio solve can be applied to a non-MetaHuman face mesh directly (vs. curve remap) is not documented on the Epic page.

## MetaHuman in UE 5.6–5.8: licence, Creator in engine, laptop cost, cold-weather clothing

### Takeaway
Since UE 5.6 (June 2025) MetaHuman Creator lives inside the engine, MetaHumans can be used in other engines and sold on Fab, and they are free under the standard UE EULA (royalty only above $1M gross). Performance on a laptop depends mostly on strand grooms and LOD; hair cards and a raised min LOD are Epic's own fixes. Cold-weather clothing is possible via the Outfit Asset (resizable) and Marvelous Designer 2025.2 USD export (UE 5.6+), with limits.

### Cited Findings
- MetaHuman left early access in UE 5.6 under the standard UE EULA; finished characters count as non-engine products usable in Unity/Godot; under $1M revenue no payment; above that a per-seat licence ($1,850/yr per one article); can sell characters/clothing/animations on Fab (88% to creator); licence prohibits using MetaHumans as AI training data; Creator, Animator and Mesh to MetaHuman share one in-engine library; autorigging and texture synthesis still run in the cloud — [Digital Production, 5 Jun 2025](https://digitalproduction.com/2025/06/05/metahumans-graduate-ready-for-unity-godot-and-the-fab-cash-register/); Epic license page "free to use for anyone making under $1 million USD in revenue" — [metahuman.com/license (search snippet)](https://www.metahuman.com/en-US/license)
- 5.8: OpenRigLogic (RigLogic + DNA libraries) released under MIT; per-platform min/max LOD; FP16 on mobile vs FP32 desktop from one asset; cooked DNA loads faster; MetaHuman Collections (experimental) for crowds — [MetaHuman 5.8 release notes](https://dev.epicgames.com/documentation/metahuman/metahuman-5-8-release-notes-in-unreal-engine?lang=en-US); [80.lv crowds](https://80.lv/articles/populate-your-ue5-8-worlds-with-metahuman-crowds)
- Performance: Epic recommends switching strand grooms to hair cards/meshes on lower-end hardware and warns forcing LOD 0/1 on many MetaHumans hurts performance; raising Min LOD disables some animation features and uses hair cards and lower-quality materials — [Epic: Controlling MetaHuman LODs](https://dev.epicgames.com/documentation/metahuman/controlling-metahuman-levels-of-detail-lods-in-unreal-engine)
- Forum reports: ~24 fps dropping to 14 fps near a MetaHuman face, LOD ~3 "drastically increases performance"; groom bindings have high render-thread cost even in Rigid mode (~2 ms saved vs skinned) — [UE forum](https://forums.unrealengine.com/t/why-are-metahumans-so-heavy-to-run/627864); [UE forum groom bindings](https://forums.unrealengine.com/t/high-render-thread-cost-of-groom-bindings-even-on-rigid-mode/2725272)
- Clothing: 5.6 parametric body + Outfit Asset that resizes garments to the body; external DCC mesh clothing can be imported — [Digital Production](https://digitalproduction.com/2025/06/05/metahumans-graduate-ready-for-unity-godot-and-the-fab-cash-register/); Marvelous Designer 2025.2 is compatible with UE 5.6+ for USD garment -> Outfit Asset ("Sized Outfit") workflow; UE 5.5 not compatible for ChaosCloth data — [Marvelous Designer support](https://support.marvelousdesigner.com/hc/en-us/articles/52699135975705-Marvelous-Designer-to-MetaHuman-USD-Garment-Integration-Workflow); in 5.6 resizable clothing does not support custom skinning, Control Rig or RBAN — [Epic: Parametric clothing](https://dev.epicgames.com/documentation/unreal-engine/getting-started-with-parametric-clothing) (via search summary); third-party resizable outfits are sold on Fab — [UE forum example](https://forums.unrealengine.com/t/andi-j-formal-classic-suit-resizable-outfit-metahuman/2835039)

### Inferences
- For a laptop/Steam target (project rule: performance first), MetaHumans only for close-up story NPCs, with hair cards, hoods/hats hiding hair, and a raised Min LOD outside cutscenes. Down suits, hoods and goggles reduce visible groom and face cost.
- A hybrid is realistic: use MetaHuman faces/bodies as the *capture and solve targets* (markerless plugin, audio-driven face), then retarget body animation to the existing lighter Mixamo/AI-rigged NPC meshes where full MetaHumans are too heavy.

### Gaps
- No measured laptop benchmark for a MetaHuman in UE 5.8 was found; profile on the target laptop.
- The $1,850/seat figure is from a third-party article; the full Epic licence text was not readable (login redirect).
- No cold-weather/expedition clothing example for MetaHuman Outfit Assets was found.

## Real devlog reports: time per minute, pitfalls, cleanup

### Takeaway
Independent, measured solo-dev reports are scarce. The one detailed 2026 pipeline write-up (a vendor blog) reports a 45-second dialogue cutscene in 4–6 hours (about 5–8 hours per finished minute) using AI video mocap + Blender cleanup + MetaHuman Animator face. Consistent pitfalls across sources: foot sliding/floating feet, jitter, hands/fingers, moving cameras, and retarget proportion errors.

### Cited Findings
- 45-s dialogue cutscene: capture 1–2 h, Blender cleanup 1–2 h, UE assembly 1–2 h = 4–6 h total; cleanup: DeepMotion Ground Contact + Physics Correction, check foot sliding/joint pops/interpenetration/drift, optional Cascadeur AutoPhysics, foot IK on contacts, Butterworth low-pass ~8 Hz for jitter; hands are the weakest area across all tools; MetaHuman Animator face is "excellent" with iPhone TrueDepth; custom (non-MetaHuman) characters need extra retargeting; retarget artifacts come from shoulder width, leg length, spine count differences. Note: published by StraySpark, which sells MCP tools used in the pipeline; numbers are unmeasured claims — [StraySpark, 24 Mar 2026](https://www.strayspark.studio/blog/ai-mocap-indie-developers-2026-comparison)
- Floating feet and worse results with moving cameras in early tests of Epic's markerless plugin; fingers unusually good — [CG Channel](https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/)
- Rokoko-based retargeting gives "large error on foot" for generated BVH (MoMask authors) — [MoMask README](https://github.com/EricGuo5513/momask-codes)
- Rokoko Create output needs Studio cleanup for foot slippage; aimed at blocking — [CG Channel](https://www.cgchannel.com/2026/04/rokoko-create-generates-full-body-animations-for-free/)
- Video mocap reviews repeatedly name foot sliding as the unsolved issue — [aiindigo Rokoko Video review](https://aiindigo.com/blog/rokoko-video-review-2026-high-fidelity-motion-capture-without-the-suit); dedicated Blender add-on for foot cleanup exists — [Foot Flow](https://foot-flow.pages.dev/)

### Inferences
- If the current cost is ~1 week per chapter and each chapter has a few minutes of NPC acting, the AI pipeline (self-recorded webcam video -> Epic markerless plugin -> foot-lock pass -> retarget; audio -> MetaHuman audio-driven face) could plausibly bring raw animation to hours per minute rather than days; most of the remaining time will be cleanup and staging, which Claude Code can partly automate (batch processing, retarget, Sequencer assembly via Python).
- Interactions with the player (hand-holding child, rope clip-ins) will need IK/Control Rig contact fixing regardless of capture source.

### Gaps
- No Reddit r/gamedev or r/unrealengine first-hand reports with timings were retrieved in this pass.
- No independent measurement of minutes-of-cleanup per minute of AI mocap for any tool.
- No reports yet from solo developers who shipped cutscenes with the 5.8 markerless plugin (released June 2026).
