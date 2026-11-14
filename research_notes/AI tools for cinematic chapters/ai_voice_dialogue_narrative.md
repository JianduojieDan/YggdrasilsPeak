# AI Voice, AI NPC Dialogue, Narrative Authoring Tools and Steam/Legal Rules for Yggdrasil's Peak (UE 5.8, solo developer, Steam), research as of 2026-11-14

Notes for the report writer:
- Sources were checked on 2026-11-14. Pages from 2024-2025 are marked [older].
- Many pricing figures come from third-party aggregators that disagree with each other. Where an official page was readable, it is preferred and labelled (official).
- NON-COMMERCIAL licences are flagged in capitals: **NON-COMMERCIAL**.

---

## 1. AI voice / text-to-speech tools: licences, emotional range, Nepali/Sherpa accents, cost per hour of finished dialogue

### Takeaway
For a commercial Steam game in late 2026, the safest high-quality route is ElevenLabs on a paid plan. Its current models (Eleven v4, previous gen v3) support Nepali and Hindi, multi-speaker "Text to Dialogue" and inline emotion tags, and cost $0.08 per 1K characters, which is roughly $4-5 of raw API cost per finished hour of speech before retakes. Among open-source models, only a few have weights licensed for commercial use (Kokoro, Dia, Orpheus with Llama terms, CosyVoice, Qwen3-TTS, Zonos, Chatterbox, Kyutai Pocket with attribution). F5-TTS, XTTS v2, Fish Audio S2 Pro, IndexTTS and VibeVoice are non-commercial or research-only. No open-source model was confirmed to support Nepali. Vendor failure is a real risk: Replica Studios shut down in 2025, Resemble AI left voice generation in August 2026, and OpenAI's TTS models shut down on January 6, 2027. Generate all lines offline and archive the WAVs in the project.

### Cited Findings

**ElevenLabs (commercial leader)**
- Plans (official, monthly): Free $0 (10k credits, ~10 min, **NO commercial licence**); Starter $6 (30k credits, ~30 min, commercial yes, instant voice cloning); Creator $22 (121k credits, ~121 min, 1 Professional Voice Clone); Pro $99 (600k credits, ~600 min); Scale $299 (1.8M, ~1,800 min, 3 seats); Business $990 (6M, ~6,000 min, 10 seats). Annual billing = 10 months' price — [ElevenLabs pricing (official)](https://elevenlabs.io/pricing)
- API price (official): v4, v3 and Multilingual v2 are $0.08 per 1K characters; Flash/Turbo, v4 Turbo and v3 Conversational are $0.04 per 1K characters. The price is the same on every tier. Sound Effects $0.12/min; Music $0.15/min (5-min limit), with "Commercial use licensing on Starter+ plans" — [ElevenLabs API pricing (official)](https://elevenlabs.io/pricing/api)
- Discrepancy: the subscription page says Pro = 600k credits (~600 min). The API page lists Pro as including 1,238,000 characters for v4/v3. These appear to be different metering systems (UI credits vs API characters); check before budgeting — [pricing](https://elevenlabs.io/pricing); [API pricing](https://elevenlabs.io/pricing/api)
- Models (official docs, current): Eleven v4 (`eleven_v4`, "highest quality and expressiveness", 90+ languages, 10,000-character limit); v4 Turbo (~100 ms, realtime); Eleven v3 (now "previous generation", 70+ languages, 5,000-character limit); Multilingual v2 (29 languages, "most stable on long-form"); Flash v2.5 (32 languages). **Nepali (ne) is in both the v4 and v3 language lists; Hindi is supported on v4, v3, Multilingual v2 and Flash v2.5; Tibetan is not listed; Sherpa is not mentioned.** Voice Design models: `eleven_ttv_v3` and `eleven_multilingual_ttv_v2` — [ElevenLabs models docs (official)](https://elevenlabs.io/docs/overview/models)
- Text to Dialogue: available on Eleven v4 and v3; "No limit on number of speakers per dialogue"; total text ≤ 2,000 characters per request. Audio tags are natural-language instructions in square brackets inside the text, e.g. [excited], [whispers], [sighs]. The tag list is open-ended — [ElevenLabs Text to Dialogue docs](https://elevenlabs.io/docs/overview/capabilities/text-to-dialogue)
- Eleven v3 launched as an alpha in June 2025 and later came to the API [older] — [ElevenLabs blog: Eleven v3](https://elevenlabs.io/blog/eleven-v3); [v3 alpha in API](https://elevenlabs.io/blog/eleven-v3-alpha-now-available-in-the-api)
- Terms: Free users "may only use the Services for non-commercial purposes"; paid users may use them commercially but must follow the Prohibited Use Policy. A separate Voice Library Addendum governs community-shared voices, and per-voice restrictions "may be disclosed within the Services." The non-EEA terms were last updated 31 March 2026 — [ElevenLabs Terms of Use (official)](https://elevenlabs.io/terms-of-use-eu) (search summary of official terms pages)
- Third-party summary (not official): paid users may put AI voices into games and other commercial products but may not resell the service itself — [terms.law summary](https://terms.law/ai-output-rights/elevenlabs/)
- Third-party claims: audio made while subscribed stays licensed after you cancel. Free-tier content needs "elevenlabs.io" or "11.ai" attribution in the title. Professional Voice Clones may only be made of your own voice — [bigvu pricing guide](https://bigvu.tv/blog/elevenlabs-pricing-2026-plan-worth/); [infinityme guide](https://infinityme.dev/blog/can-you-use-elevenlabs-commercially/); [forasoft](https://www.forasoft.com/learn/ai-for-video-engineering/articles-ai/elevenlabs-pricing-voice-cloning-no-fakes-act)
- UE integrations (third-party, all need an API key): the "ElevenLabs Assistant" editor plugin on Fab generates voiceovers in-editor (32 languages, MP3/PCM output) — [Fab listing](https://www.fab.com/listings/679ff8f4-a931-4af1-a7ad-1949c2418576). The older "Eleven Voice" plugin turns CSV/JSON data tables into SoundWave assets — [Unreal Marketplace docs](https://docs.unrealengine.com/marketplace/ko/product/eleven-voice). The Roosevelt Studios plugin does runtime or editor TTS for UE5.x — [UE forum](https://forums.unrealengine.com/t/roosevelt-studios-llc-elevenlabs-text-to-speech/2688387). A MetaHuman real-time lip-sync tutorial with ElevenLabs/OpenAI covers UE 5.6+ — [UE forum tutorial](https://forums.unrealengine.com/t/community-tutorial-metahuman-ai-real-time-lip-sync-with-elevenlabs-openai-tts-in-ue-5-6/2556987)

**Replica Studios (dead)**
- Replica Studios discontinued service on June 30, 2025 and told users to download their data. Users on the Reallusion forum reported projects stranded because the voices could no longer be accessed. Replica had previously signed a SAG-AFTRA deal (CES 2024) [older] — [MultiLingual](https://multilingual.com/replica-studios-shutdown-2025/); [Reallusion forum](https://discussions.reallusion.com/t/replica-studios-shut-down/14198)

**Resemble AI / Chatterbox (pivoted away from voice)**
- Chatterbox (original, Multilingual, Turbo) is MIT-licensed with no royalties. The Multilingual version covers 23 languages; the list includes Hindi but Nepali was not seen. It has an "emotion exaggeration" control (default 0.5; ≥0.7 is more dramatic), and output carries Resemble's PerTh watermark — [Resemble Chatterbox page](https://www.resemble.ai/learn/models/chatterbox); [HF model card](https://huggingface.co/ResembleAI/chatterbox/blob/refs%2Fpr%2F48/README.md)
- Resemble's CEO wrote (Aug 13, 2026) that the company moved its resources into deepfake detection. Resemble docs list Chatterbox Multilingual (`tts-v4`) as End of Life, and deprecated models no longer take new voice clones. A Japanese tech blog (Nov 5, 2026) says Chatterbox is now archived and unmaintained; the code and weights stay public under MIT — [Resemble blog](https://www.resemble.ai/resources/why-a-former-voice-ai-company-went-all-in-on-deepfake-detection); [Resemble model versions](https://docs.resemble.ai/getting-started/model-versions); [issoh.co.jp](https://www.issoh.co.jp/md/tech/7231)
- Cartesia's licence guide also says "Retired August 2026; no longer maintained" — [Cartesia open-source TTS guide](https://www.cartesia.ai/learn/open-source-tts)

**OpenAI TTS (avoid: being shut down)**
- `gpt-4o-mini-tts` can be steered through an `instructions` parameter covering "accent, emotional range, intonation, impressions, speed, tone, and whispering". **`gpt-4o-mini-tts`, `tts-1` and `tts-1-hd` are deprecated and shut down on January 6, 2027**; the migration target is `gpt-realtime-2.1-mini`, which is Realtime API only. OpenAI requires developers to "provide a clear disclosure to end users that the TTS voice they are hearing is AI-generated and not a human voice." Custom voices need a consent recording from the speaker — [OpenAI TTS guide (official)](https://developers.openai.com/api/docs/guides/text-to-speech)

**Microsoft Azure TTS**
- Third-party trackers put prebuilt Neural voices at about $15-16 per 1M characters and Neural HD at about $22 per 1M, with 0.5M characters free per month. The sources disagree slightly — [texttolab](https://texttolab.com/blog/azure-text-to-speech-pricing); [beri.net](https://www.beri.net/article/elevenlabs-vs-cartesia-vs-azure-tts-latency-pricing-voice-cloning-2026)
- A free AzSpeech UE plugin exists — [UE forum AzSpeech](https://forums.unrealengine.com/t/free-azspeech-plugin-text-to-speech-speech-to-text-and-more-with-microsoft-azure-cognitive-services/495394?page=14)

**Inworld AI (now a voice/TTS company)**
- Inworld retired its legacy Character Studio. Character work moved to the "Inworld Agent Runtime", which has an Unreal Character template (dialogue/emotion graphs, voice, lip-sync); Inworld TTS and voice cloning replace the old voice studio — [Inworld docs (official)](https://docs.inworld.ai/character-studio-deprecation)
- Secondary sources say Inworld repositioned itself as general real-time voice AI infrastructure, with "Realtime TTS-2" as its flagship (launch date given as May or Aug 2026; sources conflict), announced price cuts (June 10, 2026), and acquired Ultravox — [aiwiki](https://www.aiwiki.ai/wiki/inworld_ai); [Morningstar/BusinessWire](https://www.morningstar.com/news/business-wire/20260610968386/inworld-cuts-prices-to-take-down-the-biggest-wall-in-consumer-ai-cost); [RuntimeWire](https://runtimewire.com/article/inworld-acquires-ultravox-voice-agents)

**Respeecher (speech-to-speech voice conversion)**
- Respeecher's game-developer page offers a voice changer "for creatures and monsters" and "Flexible Pricing Models", but gives no public indie price. Aggregators disagree ($4-$499 editions per G2 [older, Oct 2024]; "contact sales" per Toolradar) — [Respeecher game devs](https://respeecher.com/marketplace/game-developers?hsLang=en); [G2](https://g2.com/products/respeecher/pricing); [Toolradar](https://toolradar.com/tools/respeecher/pricing)

**Open-source TTS: licence table (weights licence is what matters)**
- Kokoro-82M: Apache 2.0, commercial yes, 8 languages (English strongest; non-English "may be absent or thin"), **no voice cloning**, runs on CPU — [yaps.ai licence check, 10 Oct 2026](https://www.yaps.ai/blog/best-local-tts-models); [Cartesia](https://www.cartesia.ai/learn/open-source-tts)
- Dia / Dia2: Apache 2.0, commercial yes, **English only**. Built for dialogue: speaker tags plus nonverbal cues (laughs, sighs). The voice changes between runs unless you give an audio prompt. About 4.4 GB VRAM at bf16 (RTX 4090) — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
- Orpheus 3B: Apache 2.0 on the model card but built on Llama-3.2-3B-Instruct, **so Meta's Llama terms also apply**. English (multilingual preview); inline tags such as laugh/sigh/cough — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models); [Cartesia](https://www.cartesia.ai/learn/open-source-tts)
- CosyVoice (Fun-CosyVoice 3.0): Apache 2.0, 9 languages plus 18+ Chinese dialects, instruction control, ~150 ms streaming — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
- Qwen3-TTS (released Jan 22, 2026): Apache 2.0, commercial yes, 10 languages (no Hindi/Nepali). Clones from ~3 s of audio; plain-language emotion instructions and a VoiceDesign model; 0.6B/1.7B — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
- Zonos v0.1: Apache 2.0, 5 languages, needs 6 GB+ GPU memory — [Cartesia](https://www.cartesia.ai/learn/open-source-tts)
- Kyutai Pocket TTS: CC BY 4.0 with prohibited-use terms. Commercial use allowed **with attribution to Kyutai**; 100M parameters, runs on 2 CPU cores; no emotion control — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
- Sesame CSM-1B: Apache 2.0, a conversational model that works best with context. Commercial status was not stated in the reviewed source — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
- Chatterbox: MIT, commercial yes, watermarked output, now unmaintained (see above) — [Cartesia](https://www.cartesia.ai/learn/open-source-tts)
- **NON-COMMERCIAL / restricted:**
  - F5-TTS: MIT code, but **pretrained weights CC-BY-NC 4.0 (NON-COMMERCIAL)** — [Cartesia](https://www.cartesia.ai/learn/open-source-tts); [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
  - XTTS v2 (Coqui): **Coqui Public Model License "allows only non-commercial use of a machine learning model and its outputs" (NON-COMMERCIAL, outputs included)**. Coqui shut down (Jan 2024), so nobody can sell a commercial licence — [Cartesia](https://www.cartesia.ai/learn/open-source-tts); [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
  - Fish Audio S2 Pro (Fish Speech / OpenAudio line): **Fish Audio Research License (NON-COMMERCIAL** without a separate licence from Fish Audio). 80+ languages, 15,000+ free-form emotion tags, 5B parameters. Older Fish Speech versions were described with Apache/CC-BY-NC-SA mixes; sources conflict — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models); [ocdevel](https://ocdevel.com/blog/20250720-tts)
  - IndexTTS-2 / 2.5: **bilibili Model Use License; the README tells commercial users to contact bilibili for a separate agreement.** It has the finest emotion control of the group (8-number emotion vector, text description, or reference clip). A third-party site claims commercial use is allowed below user/revenue thresholds; unverified, and it conflicts with the README summary — [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models); [tts.ai](https://tts.ai/voices/indextts2/?lang=hi)
  - VibeVoice (Microsoft): MIT but research use only; Microsoft removed the TTS code on Sept 5, 2025 — [Cartesia](https://www.cartesia.ai/learn/open-source-tts); [yaps.ai](https://www.yaps.ai/blog/best-local-tts-models)
  - Piper: GPL-3.0 engine, with a separate licence for each voice — [Cartesia](https://www.cartesia.ai/learn/open-source-tts)

### Inferences
- **Cost per finished hour (calculation, not sourced):** one hour of speech at ~150 words/min ≈ 9,000 words ≈ ~54,000 characters.
  - ElevenLabs v4/v3 at $0.08/1K chars: ≈ $4.30 raw per finished hour. With 3-5 takes per line to pick the best performance by ear: ≈ $13-22 per finished hour.
  - The subscription equivalent: one month of Pro ($99, ~600 min of generation) covers ~10 hours of raw generation, i.e. ~2-3 finished hours after retakes.
  - A 14-hour game with perhaps 2-4 hours of spoken lines would cost roughly $100-400 in total. This is far below human voice actors, but the real cost is the developer's listening and selection time.
  - Azure at ~$16/1M chars ≈ $0.90 per finished hour, but it has less emotional range.
- **Nepali/Sherpa:** ElevenLabs is the only vendor confirmed here to support Nepali (v3/v4). Nima and his father mostly speak English with a Sherpa/Nepali accent, plus a few Nepali words. The practical method is likely ElevenLabs Voice Design with an accent described in the prompt, plus Nepali-language lines generated in v4. There is no Tibetan or Sherpa-language TTS. Lama chants and prayers should come from licensed real recordings or a consenting human speaker.
- **Tone ("epic, poetic, fate"):** ElevenLabs audio tags ([whispers], [sighs]) and multi-speaker Text to Dialogue suit radio and campfire scenes. For ghost voices, a cheap and safe approach is post-processing (reverb, pitch, granular effects) on Voice-Design voices. Respeecher-style speech-to-speech is an option if the developer performs the lines.
- **Vendor-risk rule:** Replica (2025), Resemble (2026) and OpenAI's TTS (Jan 2027) all disappeared or changed. The game should never depend on a cloud TTS at runtime. Generate offline, keep the WAVs and the generation scripts in the repo, and record the voice ID and settings per character in a manifest.
- **UE 5.8 integration (from general Unreal knowledge, not verified in this research):** pre-generated WAVs import as SoundWave assets. UE's Dialogue Voice / Dialogue Wave assets carry subtitle text, and the Localization Dashboard can gather dialogue. A Python editor script (which Claude Code can write) could batch-import a folder of WAVs and create Dialogue Waves from a CSV or Ink line-ID export.
- **Claude Code fit:** an AI coding agent can drive the ElevenLabs REST API from a Python script: read the Ink/Yarn script, generate each line with a fixed voice ID, save to `/Content/Audio/Dialogue/<char>/<lineID>.wav`, then import through UE Python. The listening and choosing must stay with the developer, who judges all sound by ear.

### Gaps
- No official confirmation of Azure Nepali neural voices (believed to exist as ne-NP voices, but no search result confirmed this) or of their price.
- Google Cloud TTS, Sesame's hosted product, and Altered Studio were not found in searches; no current licence or price data.
- The exact Voice Library Addendum text for using community-shared ElevenLabs voices in a commercial game was not retrieved. Check the restrictions on each voice's page before using one.
- Whether ElevenLabs credits for Eleven v4 cost the same as v3 in the UI (vs API) was not stated.
- No independent blind-listening comparison of v4 vs v3 vs the open models for dramatic or poetic delivery was found.

---

## 2. SAG-AFTRA 2025 video game agreement on AI voices: what it means for an indie

### Takeaway
The 2025 Interactive Media Agreement (ratified July 2025) binds only employers that sign it. Global Rule One binds union members, not developers. A non-signatory solo developer using fully synthetic voices (designed voices, not clones of any real performer) has no SAG-AFTRA obligations. Hiring a union actor, cloning a real performer, or prompting AI to imitate a named performer brings in consent, payment per generated line, and usage-report duties (via the Tiered-Budget Independent agreement for indies), and may also raise right-of-publicity issues.

### Cited Findings
- Members ratified the 2025 IMA by 95.04% to 4.96%, ending the ~320-day video game strike; the deal covers about 160,000 members [older, July 2025] — [Mondaq](https://www.mondaq.com/unitedstates/gaming/1703054/sag-aftras-new-video-game-agreement); [GamesBeat](https://gamesbeat.com/sag-aftra-union-members-approve-to-end-strike-with-game-firms-over-ai/)
- AI terms in the IMA:
  - Consent must be written and "clear and conspicuous". The employer must describe intended use, including whether the replica will drive real-time generation (e.g. a chatbot).
  - For most uses, consent cannot be obtained at hiring for future games.
  - Vocal digital replicas are paid per generated line, where a line ≈ 10 words.
  - A usage report is due within 90 days of a game's release.
  - "Independently Created Digital Replicas" (ICDR) cover replicas made from non-union material or made by prompting an AI with a performer's name; they still generally require consent and bargaining.
  - Noise reduction, pitch adjustment and similar processing are excluded.
  — [Frankfurt Kurnit (FKKS) Technology Law](https://technologylaw.fkks.com/post/102mewu/inside-the-new-sag-aftra-interactive-media-agreement-new-standards-for-ai-and-di)
- ICDR definition: a replica of a performer who previously worked under a SAG agreement in the last 3 years, "objectively identifiable" as that performer — [Davis+Gilbert](https://www.dglaw.com/sag-aftras-new-video-game-agreement/)
- The Tiered-Budget Independent IMA (TBIIMA) is designed for indies, with tiers by budget (e.g. Tier 3: $5M to <$15M) — [Davis+Gilbert](https://www.dglaw.com/sag-aftras-new-video-game-agreement/); [Legal Moves](https://legalmoveslawfirm.com/sag-aftra-ima/)
- Global Rule One forbids **members** from working for employers without a union agreement. Non-members may work non-union, but the union warns of "exploitation of your movement, voice, and likeness without your consent". Members may work on games signed to the Interim IMA or the Tiered-Budget Independent agreement — [Backstage](https://www.backstage.com/magazine/article/sag-aftras-global-rule-one-applies-interactive-video-games-44234/); [SAG-AFTRA](https://sagaftra.org/important-reminder-about-non-union-video-game-work)
- The FKKS summary does not say whether non-signatory indies are bound; it refers only to "employers covered by the IMA" — [FKKS](https://technologylaw.fkks.com/post/102mewu/inside-the-new-sag-aftra-interactive-media-agreement-new-standards-for-ai-and-di)

### Inferences
- For Yggdrasil's Peak: keep every voice fully synthetic (ElevenLabs Voice Design or permissively licensed open models). Never prompt with a real actor's name or imitate a recognisable person. Keep a consent and provenance log per character. If a human performer is used later (e.g. the developer, or a Nepali friend for authentic phrases), get written consent that names AI use explicitly.
- A union actor is very unlikely at this budget. If one is used, the TBIIMA's lowest tier and its AI clauses apply.

### Gaps
- The full TBIIMA tier list and whether the lowest tier changes the AI rules were not retrieved.
- Non-US likeness/voice law (EU AI Act transparency rules, China/Nepal rules) was not researched.

---

## 3. Runtime AI NPC conversation tools (Convai, Inworld, NVIDIA ACE, Charisma, Ubisoft, local LLM plugins)

### Takeaway
Runtime LLM NPCs are a poor fit for a fixed, tone-critical story game on a laptop. The cloud options (Convai, Inworld Agent Runtime, the Runtime AI Chatbot Integrator) cost money per interaction, need a connection, and add latency. The on-device options (NVIDIA ACE SLMs, llama.cpp plugins) compete with UE5 rendering for VRAM, and NVIDIA ACE targets RTX GPUs. Every live-generated system also triggers Steam's guardrail duties and the player hostility shown in surveys. The strongest use of LLMs here is at authoring time (Claude Code drafts Ink/Yarn lines that the developer edits). Runtime use, if any, should be limited to small optional systems.

### Cited Findings
- **Convai:** no official pricing was readable; aggregators conflict (free 100 interactions/month; paid tiers $9, $19 "indie" with engine plugin, $99, up to $1,199/month "Scale"; last verified March 2026). A review says it depends on network connectivity, which "prevents fully offline game deployment". The Unreal plugin is on Fab; a March 2026 community review says SDK 3.6.9-hotfix-2 broke long-term memory — [aitoolsatlas](https://aitoolsatlas.ai/tools/convai/pricing); [toolradar](https://toolradar.com/tools/convai/pricing); [artificial-intelligence-wiki](https://artificial-intelligence-wiki.com/industry-ai/ai-in-gaming/convai-character-ai-guide/); [convai.com/pricing (page did not render)](https://convai.com/pricing)
- **Inworld:** Character Studio is retired and the Agent Runtime SDK has an Unreal Character template. The docs do not mention on-device/offline use — [Inworld docs](https://docs.inworld.ai/character-studio-deprecation)
- **NVIDIA ACE:**
  - The ACE Game Agent SDK beta ships UE5 plugins for ASR, SLM and TTS; NVIDIA's ACE page offers plugins for **UE 5.5, 5.6 and 5.7** (5.8 not listed).
  - On-device models include Nemotron Nano SLMs (0.6B, 4B, 8B, 9B) and Qwen3.5 4B (multilingual, "201 languages and dialects").
  - Shipped uses: Total War: PHARAOH AI advisor; PUBG Ally (Mistral-NeMo-Minitron-2B); Mecha BREAK (Nemotron-4 4B, 2024) [older].
  - Positioned as "in-game AI companions that run on RTX GPUs".
  — [NVIDIA ACE for Games (official)](https://developer.nvidia.com/ace-for-games); [NVIDIA dev blog: Game Agent SDK + UE5 plugins](https://developer.nvidia.com/blog/build-on-device-ai-companions-with-the-nvidia-ace-game-agent-sdk-and-unreal-engine-5-plugins/); [NVIDIA blog Qwen3/DLSS 4.5](https://developer.nvidia.com/blog/whats-new-for-game-developers-in-nvidia-rtx-dlss-4-5-for-ue5-and-multilingual-ai-characters/); [TweakTown](https://tweaktown.com/news/112257/nvidia-launches-ace-game-agent-sdk-beta-for-in-game-ai-companions-that-run-on-rtx-gpus/index.html); [NVIDIA blog 2024](https://blogs.nvidia.com/blog/ai-decoded-gamescom-ace-nemotron-instruct)
- **Ubisoft Teammates / NEO NPC:** revealed Nov 2025 as an "experimental research project" (Snowdrop engine) building on the 2024 Neo NPCs made with NVIDIA. Early versions took more than 3 seconds to process speech, decide, respond and synthesise audio. Ubisoft describes an API with safeguards against hallucination and toxicity. Not a product available to developers — [Engadget](https://www.engadget.com/gaming/ubisoft-made-a-prototype-game-with-voice-controlled-ai-teammates-200834163.html); [Computer Weekly](https://www.computerweekly.com/news/366642940/In-video-games-of-the-future-your-AI-teammates-will-actually-listen); [Ubisoft news](https://news.ubisoft.com/en-us/article/3mWlITIuWuu0MoVuR6o8ps)
- **Local LLM UE plugins:**
  - "Runtime Local LLM" (llama.cpp) runs GGUF models fully on-device with no cloud or subscription. It offers streaming tokens, multi-turn chat, save/load and auto-summarisation, with Blueprint and C++ APIs — [UE forum](https://forums.unrealengine.com/t/runtime-local-llm-llama-cpp/2718296); [Fab](https://fab.com/s/7c8653545615)
  - "Runtime AI Chatbot Integrator" covers cloud APIs (OpenAI, Claude, Grok, etc.), Ollama for local models, and ElevenLabs TTS chunked streaming — [docs](https://georgydev-docs.pages.dev/runtime-ai-chatbot-integrator/overview); [Fab](https://fab.com/s/08d6e9e15cd1)
  - AIChatPlus supports OpenAI, Azure, Claude, Gemini and Ollama, with an offline llama.cpp option (Win64/Mac) — [UE forum](https://forums.unrealengine.com/t/plugin-aichatplus-ai-chat-integration-openai-azure-claude-gemini-ollama/2034193)
- **Shipped AI-conversation games (reception):**
  - *Whispers from the Star* (Aug 2025, open-ended AI chat, no dialogue trees): Steam "Very Positive" (~82% of ~1,618 reviews); estimated ~33-34K copies / ~$290K gross (Raijin estimates); coverage noted a "growing divide" over AI — [Steam](https://store.steampowered.com/app/3730100); [Raijin](https://raijin.gg/app/3730100/Whispers_from_the_Star/sales-revenue); [Notebookcheck](https://www.notebookcheck.net/Steam-launch-New-interactive-fiction-game-debuts-to-Very-Positive-reviews-may-divide-gamers-over-heavy-AI-integration.1087821.0.html)
  - *Suck Up!* (talk-to-AI vampire comedy): 59% of 263 reviews; estimated ~5.6K copies (Raijin) — [Raijin](https://raijin.gg/app/2726370/Suck_Up); [Steam](https://store.steampowered.com/app/2726370/Suck_Up/)
- **Charisma.ai:** no current data found (see Gaps).

### Inferences
- Live-generated NPCs conflict with the design goals of authored character arcs (Nima's bond, tell-the-truth-or-lie choices, peak-end moments) and with the "epic, poetic" tone. An LLM can drift out of tone, reveal spoilers, or break the 1:1 Everest realism.
- Laptop budget: the user's own memory notes 10-12 GB RAM use already. A 4B SLM at 4-bit needs roughly 2.5-3 GB of VRAM or RAM on top of that (general knowledge, not sourced), which works against the performance-first rule.
- A possible narrow runtime use: the "self-writing journal" could be built deterministically from templated sentences chosen by game state (authored in Ink with variables), which is also "pre-generated" for Steam. An optional on-device SLM could rephrase journal lines, but that would make the game "live-generated" with guardrail duties. Not recommended for the thesis.
- AI-written dialogue that ships (even if drafted by Claude Code and edited by the developer) is very likely "pre-generated" AI content under Steam's narrative scope and should be disclosed.

### Gaps
- Charisma.ai current pricing/status and any UE 5.8 plugin: not found.
- Official Convai pricing and per-player cost: the official page did not render.
- Real measured latency/VRAM of ACE SLMs alongside a heavy UE5 scene on a laptop GPU: not found.
- No NVIDIA ACE plugin listed for UE 5.8 yet.

---

## 4. Dialogue / narrative authoring tools and UE plugins (Articy, Ink, Yarn, Arcweave, Fab plugins), AI-agent-friendly text formats, subtitles and localisation

### Takeaway
Ink with The Chinese Room's Inkpot plugin is the best match. It is plain text an AI agent can write and diff, it is free and open-source, it was used on a shipped AAA game (Bloodlines 2), it supports UE 5.4+, and it has a runtime variable debugger. Yarn Spinner is also plain text and stronger on line IDs and localisation, but its Unreal version is still an alpha the developers say not to ship with, and it needs UE 5.7+. articy:draft X FREE (700 objects per project, commercial rights, free UE importer) and Arcweave (free tier, free UE plugin) are visual GUI tools that are less agent-friendly. Fab graph-dialogue plugins exist but store data in binary .uasset files an agent cannot easily write.

### Cited Findings
- **Ink + Inkpot:**
  - Inkpot is The Chinese Room's open-source Ink integration for UE5; Inkle credits them on the official Ink page.
  - The current README says "Unreal Engine 5.4 or later" (an older copy said 5.3).
  - It must be installed as a project plugin, not an engine plugin. Compiling uses InkleCate, which needs .NET 5.0.
  - It includes the "Blotter" in-editor debugger for viewing and setting Ink variables at runtime, and Ink List support in Blueprints. A demo project exists, and support is in Inkle's Discord #unreal-projects.
  — [Inkpot GitHub](https://github.com/The-Chinese-Room/Inkpot); [inklestudios.com/ink](https://inklestudios.com/ink/); [PC Gamer](https://www.pcgamer.com/inkles-scripting-language-ink-has-unreal-integration-thanks-to-bloodlines-2-developers-the-chinese-room/)
  - A community fork exists — [Solessfir/Inkpot](https://github.com/Solessfir/Inkpot); a UE forum thread logs issues — [Inkpot Issues](https://forums.unrealengine.com/t/inkpot-issues/2680526)
  - The Bloodlines 2 dev diary covers mission scripting in Ink — [RPG Codex](https://rpgcodex.net/article.php?id=12291)
- **Yarn Spinner for Unreal:**
  - Pre-release (alpha). The docs page shows plugin 3.2.8-alpha9 and says "We don't recommend you use it to ship a game yet".
  - Requires UE 5.7 or later for Yarn Spinner 3.3.x; 5.8 is not named but falls under "or later".
  - Free on GitHub during pre-release and "will always be available there for free"; a paid Yarn Spinner+ comes later. GitHub Sponsors at $25+ or Patreon "Scribe"+ backers get a licence to it.
  - Licence: Yarn Spinner Public License allows commercial use; its restrictions cover redistributing it inside a competing dialogue tool and **using it to train AI models**.
  - Components: Dialogue Runner, Line Presenter, Options Presenter, Line Advancer; file watcher reimport.
  — [Yarn Spinner Unreal install docs](https://www.yarnspinner.dev/docs/unreal/01-installation/)
  - Roadmap: "core is solid, shipping in 2026". January 2026: "usable in weeks, release-ready in months". June 2026: people building with the alpha — [Yarn Spinner in 2026](https://yarnspinner.dev/blog/yarn-spinner-in-2026/); [Jan '26 update](https://www.yarnspinner.dev/blog/monthly_jan_26/); [Jun '26 update](https://www.yarnspinner.dev/blog/monthly_jun_26)
- **articy:draft X:**
  - The FREE version is permanent, capped at 700 objects per project, and "comes with commercial rights included". All exports work on FREE, and the Unity/Unreal importers are free with articy:draft X.
  - The X importer on Fab requires articy:draft X / X FREE 4.1.0+. "Newest importer features are only available with the latest 3 engine versions."
  - The old articy:draft 3 importer gets no update for UE 5.6 or newer.
  — [articy FREE](https://www.articy.com/en/articydraft/free/); [itch.io FREE](https://articy-software.itch.io/articydraft-x-free); [articy Unreal importer help](https://www.articy.com/help/Integrations_Unreal.html); [Fab listing](https://www.fab.com/listings/e9edb068-c115-4fb0-a7f3-d17460b5dd28?lang=de); [articy FAQ](https://www.articy.com/en/frequently-asked-questions/)
- **Arcweave:** a free Unreal plugin imports projects directly. Arcweave says it is "completely free to use, forever"; G2 lists 3 editions from $0 to $25. Whether engine export needs a paid tier is unclear — [Arcweave Unreal plugin docs](https://arcweave.com/docs/1.0/unreal-plugin); [Arcweave pricing](https://arcweave.com/pricing); [G2](https://g2.com/products/arcweave/pricing)
- **Fab / forum UE5 dialogue plugins (vendor claims):**
  - "Dialog Plugin – easy branching dialog system" (UE 5.5-5.7) — [Fab](https://www.fab.com/listings/c9875dea-e4b9-4fc6-9f73-cd666e06d681)
  - VoxBox (bakes conversations to DataTables)
  - OGS Dialogue & Quest Master
  - BDC Dialog Backend (global value system, free demo) — [UE forum](https://forums.unrealengine.com/t/bdc-dialog-backend-professional-graph-based-dialogue-management-system/2692066)
  - Advanced Dialogue Manager (voice audio, rich text, typewriter)
  - ConvoGraph (auto save/load of visited nodes) — [UE forum](https://forums.unrealengine.com/t/astore-convograph/2846581)
  - Libretto (dialogue editor + cutscene sequence generator) — [UE forum](https://forums.unrealengine.com/t/pinch-me-games-libretto-dialogue-editor-cutscene-sequence-generator/2839932)
  - Komodobit GPT Dialogue System — [UE forum](https://forums.unrealengine.com/t/2494726)

### Inferences
- **Agent-authorable formats:** Ink (.ink) and Yarn (.yarn) are plain text that Claude Code can write, refactor, lint and diff in git. Twine's Twee format is also text (general knowledge). articy and Arcweave are GUI/database tools, editable by an agent only through JSON exports. Fab graph plugins store data in binary .uasset files, though some (VoxBox, Dialog Plugin) can be fed from DataTables/CSV.
- **Recommended pipeline (inference):**
  1. Ink via Inkpot, one .ink file per chapter.
  2. Hashtag line IDs on every line (Ink supports `#tags`), e.g. `#id:c03_nima_017 #vo #radio`.
  3. A Python script parses the compiled JSON, sends each line to ElevenLabs with a per-character voice ID, and writes WAVs named by line ID.
  4. A UE Python script imports them and builds subtitle/localisation tables.
  5. Ink variables drive truth/lie choices and journal entries.
  Yarn has built-in line tags and localisation in its Unity version, but the Unreal version is alpha (don't ship).
- **Engine-version risk:** Inkpot (5.4+), Yarn (5.7+) and articy X (latest 3 engines) should cover 5.8 in principle, but none states 5.8 explicitly. Test a compile before committing.
- The Yarn Spinner licence's "no AI training" clause is not breached by an AI agent writing .yarn scripts, but read it before relying on that.

### Gaps
- "Not Yet" dialogue plugin, Narrative/Narrative Pro (Reubs), and Lyra's CommonConversation plugin: no search results returned. Their status for UE 5.8 is unknown.
- Inkpot's explicit UE 5.8 compatibility and how it handles localisation and voice-over: not documented in the sources read.
- Chat Studio and StateTree/Mass-based dialogue: no sources found.
- Twine and Twee to UE pipelines: not researched in detail.

---

## 5. Steam policy on AI-generated content and player sentiment about AI voices

### Takeaway
Steam allows AI voices and AI-written text but requires disclosure. Since January 2024, content is split into "pre-generated" (made with AI during development, held to the same standards as other content) and "live-generated" (made while the game runs; needs described guardrails, players can report it through the overlay, and live-generated adult sexual content is banned). The January 16, 2026 revision exempts behind-the-scenes efficiency tools like code assistants and focuses on content players see or hear, including artwork, sound, narrative, localisation and store/marketing material. The disclosure text appears in the store page's "About This Game" section. Player sentiment is strongly negative, especially toward AI dialogue and voices: there is review-bombing, and even false accusations against human-voiced games.

### Cited Findings
- **January 2024 policy [older]:**
  - Pre-generated = content "created with the help of AI tools during development"; live-generated = content "created with the help of AI tools while the game is running".
  - Pre-generated content must not be illegal or infringing and must be consistent with marketing.
  - Live-generated requires describing "what kind of guardrails you're putting on your AI".
  - Disclosures appear on the store page. Players can report illegal live-generated content via the Steam overlay. Adult Only Sexual content made with live-generated AI is not allowed.
  — [PC Gamer](https://www.pcgamer.com/valve-updates-its-steam-policy-on-ai-so-it-can-release-the-vast-majority-of-games-that-use-it); [VGC](https://www.videogameschronicle.com/news/steam-pages-will-now-disclose-which-games-use-ai-generated-content/); [GamesRadar](https://gamesradar.com/valves-new-steam-policy-will-allow-the-vast-majority-of-ai-games-to-release)
- **January 16, 2026 revision:**
  - Developers no longer disclose "AI powered tools" used for workflow efficiency (e.g. code helpers).
  - They must disclose AI used "to generate content for the game" and its associated material (store page, marketing), with a text box describing the tools.
  - A checkbox covers "AI content generated during gameplay".
  — [Game Developer](https://www.gamedeveloper.com/business/valve-tweaks-and-clarifies-ai-disclosure-rules-for-steam); [PC Gamer](https://www.pcgamer.com/software/ai/steam-updates-ai-disclosure-form-to-specify-that-its-focused-on-ai-generated-content-that-is-consumed-by-players-not-efficiency-tools-used-behind-the-scenes/)
  - Pre-generated examples listed: artwork, sound, story elements, localisation, store descriptions, marketing. Steam says it is "not concerned with AI-powered tools used behind the scenes for efficiency". Developers stay responsible for keeping illegal or inappropriate live content away from players, and games without controls can be removed. The disclosure shows publicly under "About This Game" and can be updated any time — [Notebookcheck](https://www.notebookcheck.net/Steam-updates-AI-disclosure-form-requiring-developers-to-report-visible-and-in-game-AI-but-not-background-tools.1206103.0.html)
  - Low-quality or aggregator sources claim 7,300+ games had disclosed AI content by March 2026 and that there was still no official AI tag or store filter as of Sept 2026. Unverified — [tech-insider](https://tech-insider.org/steam-ai-disclosure-2026/); [dig.watch](https://dig.watch/updates/new-steam-rules-redefine-when-ai-use-must-be-disclosed)
- **Player sentiment surveys:**
  - Quantic Foundry (Oct-Dec 2025, N=1,799): 85% negative about generative AI in games, 63% picked the most negative option. "AI-generated quests/dialogue" negativity rose from 46% (2024) to 77-83%. Attitudes are less negative the further AI is from artistic or creative features — [GamesMarket](https://www.gamesmarket.global/quantic-foundry-most-gamers-are-negative-about-the-use-of-gen-ai-in-video-games); [Boing Boing](https://boingboing.net/2025/12/20/survey-finds-very-negative-attitude-toward-gen-ai-in-games.html)
  - Circana PlayerPulse: just over 25% of US players are less likely to buy a game using gen AI, and more than 50% are neutral — [Outlook Respawn](https://respawn.outlookindia.com/gaming/gaming-news/do-gamers-care-about-genai-new-data-reveals-a-silent-majority)
  - YouGov (likely 2024 [older]): gamers were twice as likely to expect worse AI performances than human ones (40% vs 18%) — [YouGov](https://business.yougov.com/content/50496-gamers-see-a-role-for-ai-in-gaming-but-not-at-the-expense-of-humans)
- **Controversies:**
  - *ARC Raiders* (Embark), Nov 2025: Eurogamer gave it 2/5, largely over its AI text-to-speech voices; actors were paid to license their voices for TTS. A March 2026 report says Embark re-recorded some AI lines with human actors and that the AI voices were meant for lines "not essential to immersion" — [80.lv](https://80.lv/articles/eurogamer-s-2-5-arc-raiders-review-splits-the-gaming-community/); [GamesRadar](https://www.gamesradar.com/games/third-person-shooter/arc-raiders-devs-dont-use-generative-ai-cco-says-ai-text-to-speech-is-used-to-help-make-rich-worlds-while-keeping-the-studio-small-but-theres-no-end-goal-in-replacing-any-actors); [players.com.ua](https://players.com.ua/en/news/arc-raiders-replaces-some-ai-voice-lines-with-real-actors/)
  - *The Brew Barons* (aerial open-world game) received many negative Steam reviews for its AI voices — [Gamepressure](https://www.gamepressure.com/newsroom/an-aerial-open-world-game-has-flopped-on-steam-for-its-use-of-ai/zf6a44)
  - An indie developer removed AI voice lines from *Future? No Thanks!* after backlash — [PC Gamer](https://www.pcgamer.com/gaming-industry/game-development/i-drank-from-the-chalice-of-blood-indie-dev-scrubs-his-game-of-ai-generated-voice-lines-having-previously-described-the-technology-as-anti-human-and-anti-art/)
  - *Luminary* (Sept 2026) was review-bombed as "AI voice", but it was the developer's wife with effects applied ("AI witch hunt" debate) — [Dexerto](https://www.dexerto.com/gaming/indie-dev-accused-of-using-ai-voice-acting-in-steam-game-reveals-it-was-actually-his-wife-3410085/); [ixbt.games](https://ixbt.games/en/news/2026/09/13/435426-razrabotcika-luminary-kritikovali-za-neiroslop-ozvucku-no-okazalos-cto-eto-golos-ego-zeny.html)
  - *Shrine's Legacy* was falsely accused of AI use (Apr 2026) — [Plagiarism Today](https://www.plagiarismtoday.com/2026/04/07/game-developer-falsely-accused-of-using-ai/)
  - *Vapor World: Over the Mind* removed AI cutscenes but stayed at ~67% negative — [Everything Edinburgh](https://everythingedinburgh.com/games/news/vapor-world-steam-ai-cutscene-backlash/)
  - Debate about the AI disclosure punishing "naive honesty" — [Gamepressure](https://www.gamepressure.com/newsroom/discussion-about-ai-label-on-steam-continues-the-dev-is-being-pun/ze8ab3)
  - "Active Matter" AI notice criticised (Sept 2026) — [GamingOnLinux](https://www.gamingonlinux.com/2026/09/active-matter-developers-put-up-an-incredibly-distasteful-ai-notice-on-steam/)

### Inferences
- For Yggdrasil's Peak, AI voices and AI-drafted dialogue both count as "pre-generated" content to disclose. Keeping everything pre-generated avoids the live-generated guardrail duties and the overlay-reporting risk.
- Disclosure wording matters. A specific, honest note (e.g. "character voices were synthesised from designed, non-cloned voices and directed line by line by the developer; no real person's voice was copied") is likely better received than a vague one (inference from the controversies above).
- Hedges against sentiment risk: radio-filtered presentation (which hides TTS artefacts), keeping the most emotional lines (e.g. the 8-year-old cousin, Nima's key moments) as candidates for later human recording, and an "AI voices off / subtitles only" option.

### Gaps
- The exact current Steamworks Content Survey text (Valve's own page) could not be fetched; the details above come from press coverage.
- No 2026 survey specific to AI *voice acting* (rather than gen AI in general) was found.
- No indie game *praised* specifically for its AI voices was found. *Whispers from the Star* is praised overall despite AI chat.

---

## 6. Shipped examples and radio-only / voice-only storytelling techniques that cut cost

### Takeaway
The proven low-cost template is Firewatch: one main voice heard over a walkie-talkie, conversations triggered by what the player looks at, and choices of what to say back. This fits Dr. Hale on the radio and Nima as the climbing partner. The voice is mostly heard, not seen, so there is no lip-sync and no faces to animate, and the radio filter masks small synthesis artefacts. Shipped AI-voice examples run from backlash (ARC Raiders, Brew Barons) to acceptance when AI is the game's premise (Whispers from the Star).

### Cited Findings
- Firewatch: looking at an object or location lets Henry pull out the walkie-talkie and talk to Delilah, with several dialogue choices. A PC Gamer reviewer noted this structure makes it "relatively cheap and easy to throw new dialogue" into the game. Delilah was voiced by Cissy Jones. Campo Santo drew on The Walking Dead's finding that players connect more with lines they choose to deliver — [PC Gamer](https://www.pcgamer.com/firewatchs-wyoming-fosters-a-bigger-weirder-adventure-than-i-expected); [Inverse](https://www.inverse.com/article/11478-firewatch-showcases-the-power-of-dialogue-in-video-games); [Game Informer](https://gameinformer.com/b/news/archive/2014/03/17/sean-vanaman-speaks-about-campo-santos-goals-for-firewatch); [Hardcore Gamer: Cissy Jones](https://hardcoregamer.com/features/interviews/cissy-jones-on-being-the-voice-of-firewatch/190152/)
- GDC talks: "Do You Copy? Dialog Systems and Tools in Firewatch" (Patrick Ewing, Campo Santo, with William Armstrong, Unity; GDC 2017) and "Interactive Story Without Challenge Mechanics: The Design of Firewatch" (Chris Remo, GDC 2019) — [GDC Vault 2017](https://gdcvault.com/play/1024000/Do-You-Copy-Dialog-System); [GDC Vault 2019](https://www.gdcvault.com/play/1026087/Interactive-Story-Without-Challenge-Mechanics)
- A Twinfinite headline reports Firewatch had a 1,000-page script (not opened) — [Twinfinite](https://twinfinite.net/news/firewatch-has-a-1000-page-script-says-designer/)
- Note on a conflicting claim: one search summary said "Henry doesn't speak aloud". From general knowledge, Henry is voiced (by Rich Sommer) and the player picks his lines. Treat the search claim as wrong; verify on Firewatch's Wikipedia cast list — [Wikipedia](https://www.wikipedia.org/wiki/Firewatch)
- AI examples: ARC Raiders (AI TTS licensed from paid actors, used for non-core lines; backlash, then partial re-recording); Whispers from the Star (AI chat as the premise, Very Positive); The Brew Barons (flop over AI voices) — see sources in sections 3 and 5.

### Inferences
- Cast mapping (inference):
  - Dr. Hale is radio-only (the Firewatch model).
  - The foreign team is heard mostly over radio, with short in-person moments at camps.
  - The village aunt, cousin and lama appear in flashback or prologue scenes, possibly delivered as voice over still images or journal pages (the Kentucky Route Zero / text-first approach) to save animation cost.
  - Ghosts are processed whispers.
  - The uncle could be a recorded tape or letter.
  This keeps faces and lip-sync to a minimum (Nima in person only).
- The radio channel suits short, punchy lines and silence, which matches "epic, poetic, fate" better than long exposition.
- A text-only fallback for certain characters (journal and letters) cuts voice volume and AI-voice exposure further.

### Gaps
- No sources were retrieved for Half-Life's radio design or Kentucky Route Zero's text-first technique. No Firewatch budget or cost figures were found.
- No indie post-mortem specifically about shipping an ElevenLabs-voiced story game was found.
