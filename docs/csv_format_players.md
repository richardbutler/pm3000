# Players CSV Format for FIFA Import Tool

## Required Columns

| Column | Type | Description | Range/Format |
|--------|------|-------------|--------------|
| player_id | integer | Unique player identifier | - |
| club_id | integer | Links to club_id in clubs CSV | - |
| short_name | string | Player name (surname or display name) | 16 chars |
| age | integer | Player age | 16-34 |
| player_positions | string | Position codes (GK, DF, MF, FW) | - |
| overall | integer | Overall rating | 0-99 |
| preferred_foot | string | Left, Right, or Both | - |

## Stats (all 0-99)

| Column | Type | Description |
|--------|------|-------------|
| pace | integer | Pace/speed |
| shooting | integer | Shooting ability |
| passing | integer | Passing ability |
| dribbling | integer | Dribbling/ball control |
| defending | integer | Defensive ability |
| physic | integer | Physical strength |
| attacking_heading_accuracy | integer | Heading ability |

## Goalkeeper Stats (0-99)

| Column | Type | Description |
|--------|------|-------------|
| goalkeeping_diving | integer | GK diving |
| goalkeeping_handling | integer | GK handling |
| goalkeeping_kicking | integer | GK kicking |
| goalkeeping_positioning | integer | GK positioning |
| goalkeeping_reflexes | integer | GK reflexes |
| goalkeeping_speed | integer | GK speed |

## Optional Detailed Stats

All the FIFA detailed stats are supported (see existing FIFA tool documentation):
- attacking_crossing, attacking_finishing, attacking_short_passing, attacking_volleys
- skill_dribbling, skill_ball_control, skill_curve, skill_fk_accuracy, skill_long_passing
- movement_acceleration, movement_sprint_speed, movement_agility, movement_reactions, movement_balance
- power_shot_power, power_jumping, power_stamina, power_strength, power_long_shots
- mentality_interceptions, mentality_positioning, mentality_vision, mentality_penalties, mentality_aggression, mentality_composure
- defending_marking_awareness, defending_standing_tackle, defending_sliding_tackle

## Optional Contract Data

| Column | Type | Description |
|--------|------|-------------|
| club_contract_valid_until_year | integer | Contract expiry year |
| wage_eur | integer | Weekly wage (not used in PM3) |

## Optional Loan Data

| Column | Type | Description |
|--------|------|-------------|
| club_loaned_from | string | Parent club name (if on loan) |

Note: Loan data requires `--import-loans` flag and may cause issues with new games.

## Example

```csv
player_id,club_id,short_name,age,player_positions,overall,preferred_foot,pace,shooting,passing,dribbling,defending,physic,attacking_heading_accuracy,goalkeeping_diving,goalkeeping_handling,goalkeeping_kicking,goalkeeping_positioning,goalkeeping_reflexes,goalkeeping_speed
1,1,Ramsdale,25,GK,82,Right,40,15,55,30,20,60,45,83,80,75,82,85,50
2,1,Saliba,22,DF,85,Right,75,45,72,70,88,80,78,10,10,10,10,10,10
3,1,Saka,22,MF FW,87,Left,88,82,80,87,45,65,68,10,10,10,10,10,10
```

## Notes

- Maximum 3,932 players total (244 clubs × 16 players = 3,904)
- Each club should have at least 2 goalkeepers
- Players sorted by overall rating (top 16 per club imported)
- Ages outside 16-34 will be clamped
- Position codes: GK (goalkeeper), DF (defender), MF (midfielder), FW (forward)
