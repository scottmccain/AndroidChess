#include "chess.h"
#include "data.h"
/* last modified 08/17/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Evaluate() is used to evaluate the chess board.  Broadly, it addresses    *
 *   four (4) distinct areas:  (1) material score which is simply a summing of *
 *   piece types multiplied by piece values;  (2) pawn scoring which considers *
 *   placement of pawns and also evaluates passed pawns, particularly in end-  *
 *   game situations;  (3) piece scoring which evaluates the placement of each *
 *   piece as well as things like piece mobility;  (4) king safety which       *
 *   considers the pawn shelter around the king along with material present to *
 *   facilitate an attack.                                                     *
 *                                                                             *
 *******************************************************************************
 */
int Evaluate(TREE * RESTRICT tree, int ply, int wtm, int alpha, int beta) {
  PAWN_HASH_ENTRY *ptable;
  PXOR *pxtable;
  int score, side, can_win = 3;
  int phase, lscore, cutoff;

/*
 **********************************************************************
 *                                                                    *
 *  First thing we do is if -DSKILL was passed in as to the compiler, *
 *  we burn some time to slow the search down, then we fall into the  *
 *  normal evaluation code.                                           *
 *                                                                    *
 **********************************************************************
 */
#if defined(SKILL)
  if (skill < 100) {
    int i, j;
    for (i = 0; i < burnc[skill / 10] && !abort_search; i++)
      for (j = 1; j < 10 && !abort_search; j++)
        burner[j - 1] = burner[j - 1] * burner[j];
    if (TimeCheck(tree, 1))
      abort_search = 1;
  }
#endif
/*
 **********************************************************************
 *                                                                    *
 *  Initialize.  If the meterial score is way below alpha or way      *
 *  above beta (way above means so far above it is very unlikely the  *
 *  positional score can bring the total score back into the alpha /  *
 *  beta window) then we take what is known as a "lazy evaluation     *
 *  exit" and avoid the computational cost of a full evaluation in a  *
 *  position where one side is way ahead or behind in material.       *
 *                                                                    *
 **********************************************************************
 */
  cutoff = (TotalPieces(white, occupied) && TotalPieces(black, occupied))
      ? KNIGHT_VALUE : ROOK_VALUE;
  lscore = MaterialSTM(wtm);
  if (lscore + cutoff < alpha)
    return alpha;
  if (lscore - cutoff > beta)
    return beta;
  tree->evaluations++;
  tree->score_mg = 0;
  tree->score_eg = 0;
  EvaluateMaterial(tree, wtm);
/*
 **********************************************************************
 *                                                                    *
 *  Check for draws where one side seems to be ahead, but has no      *
 *  actual winning chances.  One simple example is a king, bishop and *
 *  rook pawn, with the wrong colored bishop and the enemy king too   *
 *  close to the promotion square.                                    *
 *                                                                    *
 *  The variable "can_win" uses 2 bits.  If White can actually win in *
 *  this position, bit 1 is set.  If Black can actually win in this   *
 *  position, bit 0 is set.  If both sides can win, both bits are     *
 *  set.  This is used later to drag the score closer to a draw score *
 *  if the side with the better score can't actually win.             *
 *                                                                    *
 *  Note that we only set these bits in minimal material positions    *
 *  (both sides have < 13 points of material total).  Otherwise we    *
 *  assume normal scoring should apply.                               *
 *                                                                    *
 **********************************************************************
 */
  if (TotalPieces(white, occupied) < 13 && TotalPieces(black, occupied) < 13)
    for (side = black; side <= white; side++)
      if (!EvaluateWinningChances(tree, side, wtm))
        can_win ^= (1 << side);
/*
 **********************************************************************
 *                                                                    *
 *  Determine if this position should be evaluated to force mate      *
 *  (neither side has pawns) or if it should be evaluated normally.   *
 *                                                                    *
 *  Note the special case of no pawns, one side is ahead in total     *
 *  material, but the game is a hopeless draw.  KRN vs KR is one      *
 *  example.  If EvaluateWinningChances() determines that the side    *
 *  with extra material can not win, the score is pulled closer to a  *
 *  draw although it can not collapse completely to the drawscore as  *
 *  it is possible to lose KRB vs KR if the KR side lets the king get *
 *  trapped on the edge of the board.                                 *
 *                                                                    *
 **********************************************************************
 */
  tree->all_pawns = Pawns(black) | Pawns(white);
  if (!tree->all_pawns) {
    if (TotalPieces(white, occupied) > 3 || TotalPieces(black, occupied) > 3) {
      if (Material > 0)
        EvaluateMate(tree, white);
      else if (Material < 0)
        EvaluateMate(tree, black);
      if (tree->score_eg > DrawScore(1) && !(can_win & 2))
        tree->score_eg = tree->score_eg / 16;
      if (tree->score_eg < DrawScore(1) && !(can_win & 1))
        tree->score_eg = tree->score_eg / 16;
#if defined(SKILL)
      if (skill < 100)
        tree->score_eg =
            skill * tree->score_eg / 100 + ((100 -
                skill) * PAWN_VALUE * (uint64_t) Random32() /
            0x100000000ull) / 100;
#endif
      return (wtm) ? tree->score_eg : -tree->score_eg;
    }
  }
/*
 **********************************************************************
 *                                                                    *
 *  Now evaluate pawns.  If the pawn hash signature has not changed   *
 *  from the last entry to Evaluate() then we already have everything *
 *  we need in the pawn hash entry.  In this case, we do not need to  *
 *  call EvaluatePawns() at all.  EvaluatePawns() does all of the     *
 *  analysis for information specifically regarding only pawns.  In   *
 *  many cases, it merely records the presence/absence of positional  *
 *  pawn features because those features also depends on pieces.      *
 *  Note that anything put into EvaluatePawns() can only consider the *
 *  placement of pawns.  Kings or other pieces can not influence the  *
 *  score because those pieces are not hashed into the pawn hash      *
 *  signature.  Violating this principle leads to lots of very        *
 *  difficult and challenging debugging problems.                     *
 *                                                                    *
 **********************************************************************
 */
  else {
    if (PawnHashKey == tree->pawn_score.key) {
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
/*
 ************************************************************
 *                                                          *
 *  First check to see if this position has been handled    *
 *  before.  If so, we can skip the work saved in the pawn  *
 *  hash table.                                             *
 *                                                          *
 ************************************************************
 */
    else {
      ptable = pawn_hash_table + (PawnHashKey & pawn_hash_mask);
      pxtable = (PXOR *) & (tree->pawn_score);
      tree->pawn_score = *ptable;
      tree->pawn_score.key ^=
          pxtable->entry[1] ^ pxtable->entry[2] ^ pxtable->entry[3];
      if (tree->pawn_score.key != PawnHashKey) {
        tree->pawn_score.key = PawnHashKey;
        tree->pawn_score.score_mg = 0;
        tree->pawn_score.score_eg = 0;
        for (side = black; side <= white; side++)
          EvaluatePawns(tree, side);
        ptable->key =
            pxtable->entry[0] ^ pxtable->entry[1] ^ pxtable->
            entry[2] ^ pxtable->entry[3];
        memcpy((char *) ptable + 8, (char *) &(tree->pawn_score) + 8, 20);
      }
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
/*
 **********************************************************************
 *                                                                    *
 *  If there are any passed pawns, first call EvaluatePassedPawns()   *
 *  to evaluate them.  Then, if one side has a passed pawn and the    *
 *  other side has no pieces, call EvaluatePassedPawnRaces() to see   *
 *  if the passed pawn can be stopped from promoting.                 *
 *                                                                    *
 **********************************************************************
 */
    if (tree->pawn_score.passed[black] || tree->pawn_score.passed[white]) {
      for (side = black; side <= white; side++)
        if (tree->pawn_score.passed[side])
          EvaluatePassedPawns(tree, side, wtm);
      if ((TotalPieces(white, occupied) == 0 &&
              tree->pawn_score.passed[black])
          || (TotalPieces(black, occupied) == 0 &&
              tree->pawn_score.passed[white]))
        EvaluatePassedPawnRaces(tree, wtm);
    }
  }
/*
 **********************************************************************
 *                                                                    *
 *  Call EvaluateCastling() to evaluate castling potential.  Note we  *
 *  only do this when that side has not castled at the root.          *
 *                                                                    *
 **********************************************************************
 */
  for (side = black; side <= white; side++)
    if (Castle(1, side) > 0)
      EvaluateCastling(tree, ply, side);
/*
 **********************************************************************
 *                                                                    *
 *  The "dangerous" flag simply indicates whether that side has       *
 *  enough material to whip up a mating attack if the other side is   *
 *  careless (Q + minor or better, or RR + R or better).              *
 *                                                                    *
 **********************************************************************
 */
  tree->dangerous[white] = (Queens(white) && TotalPieces(white, occupied) > 9)
      || (TotalPieces(white, rook) > 1 && TotalPieces(white, occupied) > 15);
  tree->dangerous[black] = (Queens(black) && TotalPieces(black, occupied) > 9)
      || (TotalPieces(black, rook) > 1 && TotalPieces(black, occupied) > 15);
/*
 **********************************************************************
 *                                                                    *
 *  Then evaluate pieces.                                             *
 *                                                                    *
 *  Note 1:  if the current score is way below alpha or way above     *
 *  beta, there is no point in doing the piece scoring, so we take a  *
 *  "lazy" approach and skip pieces in that case.                     *
 *                                                                    *
 *  Note 2:  We MUST evaluate kings last, since their scoring depends *
 *  on the tropism scores computed by the other piece evaluations.    *
 *                                                                    *
 **********************************************************************
 */
  phase =
      Min(62, TotalPieces(white, occupied) + TotalPieces(black, occupied));
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  lscore = (wtm) ? score : -score;
  cutoff = (tree->dangerous[white]
      || tree->dangerous[black]) ? 114 + phase : 102;
  if (lscore + cutoff > alpha && lscore - cutoff < beta) {
    tree->tropism[white] = 0;
    tree->tropism[black] = 0;
    for (side = black; side <= white; side++)
      EvaluateKnights(tree, side);
    for (side = black; side <= white; side++)
      EvaluateBishops(tree, side);
    for (side = black; side <= white; side++)
      EvaluateRooks(tree, side);
    for (side = black; side <= white; side++)
      EvaluateQueens(tree, side);
    for (side = black; side <= white; side++)
      EvaluateKing(tree, ply, side);
  }
/*
 **********************************************************************
 *                                                                    *
 *  Caclulate the final score, which is interpolated between the      *
 *  middlegame score and endgame score based on the material left on  *
 *  the board.                                                        *
 *                                                                    *
 *  Adjust the score if one side can't win, but the score actually    *
 *  favors that side significantly.                                   *
 *                                                                    *
 **********************************************************************
 */
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  score = EvaluateDraws(tree, ply, can_win, score);
#if defined(SKILL)
  if (skill < 100)
    score =
        skill * score / 100 + ((100 -
            skill) * PAWN_VALUE * (uint64_t) Random32() / 0x100000000ull) /
        100;
#endif
  return (wtm) ? score : -score;
}

/* last modified 08/17/14 */
/*
 *******************************************************************************
 *                                                                             *
 *  EvaluateBishops() is used to evaluate bishops.                             *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateBishops(TREE * RESTRICT tree, int side) {
  uint64_t temp, moves;
  int square, special, i, mobility;
  int score_eg = 0, score_mg = 0, enemy = Flip(side);
/*
 ************************************************************
 *                                                          *
 *  First, locate each bishop and add in its piece/square   *
 *  score.                                                  *
 *                                                          *
 ************************************************************
 */
  for (temp = Bishops(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    score_mg += bval[mg][side][square];
    score_eg += bval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate for "outposts" which is a bishop that can't be *
 *  driven off by an enemy pawn, and which is supported by  *
 *  a friendly pawn.                                        *
 *                                                          *
 *  If the enemy has NO minor to take this bishop, then     *
 *  increase the bonus.                                     *
 *                                                          *
 *  Note that the bishop_outpost array is overloaded to     *
 *  serve a dual prupose.  A negative value is used to flag *
 *  squares A7 and H7 to test for a trapped bishop.  This   *
 *  is done for speed.                                      *
 *                                                          *
 ************************************************************
 */
    special = bishop_outpost[side][square];
    if (special) {
      if (special > 0) {
        if (!(mask_no_pattacks[enemy][square] & Pawns(enemy))) {
          if (pawn_attacks[enemy][square] & Pawns(side)) {
            special += special / 2;
            if (!Knights(enemy) && !(Color(square) & Bishops(enemy)))
              special += bishop_outpost[side][square];
          }
          score_eg += special;
          score_mg += special;
        }
      }
/*
 ************************************************************
 *                                                          *
 *  Check to see if the bishop is trapped at a7 or h7 with  *
 *  a pawn at b6 or g6 that has trapped the bishop.         *
 *                                                          *
 ************************************************************
 */
      else {
        if (square == sqflip[side][A7]) {
          if (SetMask(sqflip[side][B6]) & Pawns(enemy)) {
            score_eg -= bishop_trapped;
            score_mg -= bishop_trapped;
          }
        } else if (square == sqflip[side][H7]) {
          if (SetMask(sqflip[side][G6]) & Pawns(enemy)) {
            score_eg -= bishop_trapped;
            score_mg -= bishop_trapped;
          }
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Mobility counts the number of squares the bishop        *
 *  attacks, excluding squares with friendly pieces, and    *
 *  weighs each square according to centralization.         *
 *                                                          *
 ************************************************************
 */
    mobility = BishopMobility(square, OccupiedSquares);
    if (mobility < 0 && (pawn_attacks[enemy][square] & Pawns(side))
        && (File(square) == FILEA || File(square) == FILEH))
      mobility -= 8;
    score_mg += mobility;
    score_eg += mobility;
/*
 ************************************************************
 *                                                          *
 *  Check for pawns on both wings, which makes a bishop     *
 *  even more valuable against an enemy knight              *
 *                                                          *
 ************************************************************
 */
    if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc) {
      score_mg += bishop_with_wing_pawns[mg];
      score_eg += bishop_with_wing_pawns[eg];
    }
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      moves = king_attacks[KingSQ(enemy)];
      i = ((bishop_attacks[square] & moves) &&
          ((BishopAttacks(square, OccupiedSquares & ~Queens(side))) & moves))
          ? 1 : Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_b[i];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 08/06/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateCastling() is called when "side" has not castled at the root.     *
 *   Its main purpose is to determine if it has either castled somewhere in    *
 *   the tree, or else has lost all (or some) castling rights, which reduces   *
 *   options significantly.                                                    *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateCastling(TREE * RESTRICT tree, int ply, int side) {
  int enemy = Flip(side), oq, score_mg = 0;;

/*
 ************************************************************
 *                                                          *
 *  If the king castled during the search, we are done and  *
 *  we leave it to EvaluateKings() to figure out how safe   *
 *  it is.  If it has not castled, we give a significant    *
 *  penalty if the king moves since that loses all castling *
 *  rights, otherwise we give a smaller penalty for moving  *
 *  a rook and giving up castling rights to that side of    *
 *  the board.  The penalty is always increased if the      *
 *  opponent has a queen since the position is much more    *
 *  dangerous.                                              *
 *                                                          *
 ************************************************************
 */
  oq = (Queens(enemy)) ? 3 : 1;
  if (Castle(ply, side) != Castle(1, side)) {
    if (Castle(ply, side) == 0)
      score_mg -= oq * development_losing_castle;
    else if (Castle(ply, side) > 0)
      score_mg -= (oq * development_losing_castle) / 2;
  } else
    score_mg -= oq * development_not_castled;
  tree->score_mg += sign[side] * score_mg;
}

/* last modified 08/18/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDraws() is used to adjust the score based on whether the side     *
 *   that appears to be better according the computed score can actually win   *
 *   the game or not.  If the answer is "no" then the score is reduced         *
 *   significantly to reflect the lack of winning chances.                     *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDraws(TREE * RESTRICT tree, int ply, int can_win, int score) {
/*
 ************************************************************
 *                                                          *
 *  If the ending has only bishops of opposite colors, the  *
 *  score is pulled closer to a draw.                       *
 *                                                          *
 *  If this is a pure BOC ending, it is very drawish unless *
 *  one side has at least 4 pawns.  More pawns makes it     *
 *  harder for a bishop and king to stop them all from      *
 *  advancing.                                              *
 *                                                          *
 *  If the following are both true:                         *
 *                                                          *
 *    black and white have less than a queen left (pieces   *
 *    only).                                                *
 *                                                          *
 *    both have one bishop and they are opposite colored.   *
 *                                                          *
 *  then either                                             *
 *                                                          *
 *    (a) both have just one bishop, both have less than 4  *
 *    pawns or one side has only one more pawn than the     *
 *    other side then score is divided by 2 with draw score *
 *    added in; or                                          *
 *                                                          *
 *    (b) pieces are equal, then score is reduced by 25%    *
 *    with draw score added in.                             *
 *                                                          *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, occupied) <= 8 && TotalPieces(black, occupied) <= 8) {
    if (TotalPieces(white, bishop) == 1 && TotalPieces(black, bishop) == 1)
      if (square_color[LSB(Bishops(black))] !=
          square_color[LSB(Bishops(white))]) {
        if (TotalPieces(white, occupied) == 3 &&
            TotalPieces(black, occupied) == 3 &&
            ((TotalPieces(white, pawn) < 4 && TotalPieces(black, pawn) < 4)
                || Abs(TotalPieces(white, pawn) - TotalPieces(black,
                        pawn)) < 2))
          score = score / 2 + DrawScore(1);
        else if (TotalPieces(white, occupied) == TotalPieces(black, occupied))
          score = 3 * score / 4 + DrawScore(1);
      }
  }
/*
 ************************************************************
 *                                                          *
 *  Final score adjustment.  If the score says white is     *
 *  better, but can_win says white can not win, or if the   *
 *  score says black is better, but can_win says black can  *
 *  not win, then we divide the score by 16, and then add   *
 *  in the draw score.  If the can_win says neither side    *
 *  can win, we just set the score to draw score and exit.  *
 *                                                          *
 *  We introduce a "weariness factor" when the irreversible *
 *  move counter approaches 50 moves, and drag the score    *
 *  toward zero to encourage making progress by a non-      *
 *  reversible move to reset the counter and get us away    *
 *  from the impending draw.                                *
 *                                                          *
 ************************************************************
 */
  if (can_win != 3) {
    if (can_win & 1) {
      if (score > DrawScore(1))
        score = score / 16 + DrawScore(1);
    } else if (can_win & 2) {
      if (score < DrawScore(1))
        score = score / 16 + DrawScore(1);
    } else
      score = DrawScore(1);
  }
/*
 ************************************************************
 *                                                          *
 *  If we are running into the 50-move rule, then start     *
 *  dragging the score toward draw.  This is the idea of a  *
 *  "weariness factor" as mentioned by Dave Slate many      *
 *  times.  This avoids slamming into a draw at move 50 and *
 *  having to move something quickly, rather than slowly    *
 *  discovering that the score is dropping and that pushing *
 *  a pawn or capturing something will cause it to go back  *
 *  to its correct value a bit more smoothly.               *
 *                                                          *
 ************************************************************
 */
  if (Reversible(ply) > 80) {
    int closeness = 101 - Reversible(ply);

    score = DrawScore(1) + score * closeness / 20;
  }
  return score;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateHasOpposition() is used to determine if one king stands in        *
 *   "opposition" to the other.  If the kings are opposed on the same file or  *
 *   else are opposed on the same diagonal, then the side not-to-move has the  *
 *   opposition and the side-to-move must give way.                            *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateHasOpposition(int on_move, int king, int enemy_king) {
  int file_distance, rank_distance;

  file_distance = FileDistance(king, enemy_king);
  rank_distance = RankDistance(king, enemy_king);
  if (rank_distance < 2)
    return 1;
  if (on_move) {
    if (rank_distance & 1)
      rank_distance--;
    if (file_distance & 1)
      file_distance--;
  }
  if (!(file_distance & 1) && !(rank_distance & 1))
    return 1;
  return 0;
}

/* last modified 06/12/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKing() is used to evaluate a king.                                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKing(TREE * RESTRICT tree, int ply, int side) {
  int score_eg = 0, score_mg = 0, defects;
  int ksq = KingSQ(side), enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First, check for where the king should be if this is an *
 *  endgame.  Ie with pawns on one wing, the king needs to  *
 *  be on that wing.  With pawns on both wings, the king    *
 *  belongs in the center.                                  *
 *                                                          *
 ************************************************************
 */
  if (tree->all_pawns) {
    if (tree->all_pawns & mask_efgh && tree->all_pawns & mask_abcd)
      score_eg += kval_n[side][ksq];
    else if (tree->all_pawns & mask_efgh)
      score_eg += kval_k[side][ksq];
    else
      score_eg += kval_q[side][ksq];
  }
/*
 ************************************************************
 *                                                          *
 *  Do castle scoring, if the king has castled, the pawns   *
 *  in front are important.  If not castled yet, the pawns  *
 *  on the kingside should be preserved for this.           *
 *                                                          *
 ************************************************************
 */
  if (tree->dangerous[enemy]) {
    defects = 0;
    if (Castle(ply, side) <= 0) {
      if (File(ksq) >= FILEE) {
        if (File(ksq) > FILEE)
          defects = tree->pawn_score.defects_k[side];
        else
          defects = tree->pawn_score.defects_e[side];
      } else {
        if (File(ksq) < FILED)
          defects = tree->pawn_score.defects_q[side];
        else
          defects = tree->pawn_score.defects_d[side];
      }
    } else {
      if (Castle(ply, side) == 3)
        defects =
            Min(Min(tree->pawn_score.defects_k[side],
                tree->pawn_score.defects_e[side]),
            tree->pawn_score.defects_q[side]);
      else if (Castle(ply, side) == 1)
        defects =
            Min(tree->pawn_score.defects_k[side],
            tree->pawn_score.defects_e[side]);
      else
        defects =
            Min(tree->pawn_score.defects_q[side],
            tree->pawn_score.defects_e[side]);
      if (defects < 3)
        defects = 3;
    }
/*
 ************************************************************
 *                                                          *
 *  Fold in the king tropism and king pawn shelter scores   *
 *  together.                                               *
 *                                                          *
 ************************************************************
 */
    if (tree->tropism[enemy] < 0)
      tree->tropism[enemy] = 0;
    else if (tree->tropism[enemy] > 15)
      tree->tropism[enemy] = 15;
    if (defects > 15)
      defects = 15;
    score_mg -= king_safety[defects][tree->tropism[enemy]];
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKingsFile computes defects for a file, based on whether the file  *
 *   is open or half-open.  If there are friendly pawns still on the file,     *
 *   they are penalized for advancing in front of the king.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKingsFile(TREE * RESTRICT tree, int whichfile, int side) {
  int defects = 0, file;
  int enemy = Flip(side);

  for (file = whichfile - 1; file <= whichfile + 1; file++)
    if (!(file_mask[file] & tree->all_pawns))
      defects += open_file[file];
    else {
      if (!(file_mask[file] & Pawns(enemy)))
        defects += half_open_file[file] / 2;
      else
        defects +=
            pawn_defects[side][Rank(Advanced(enemy,
                    file_mask[file] & Pawns(enemy)))];
      if (!(file_mask[file] & Pawns(side)))
        defects += half_open_file[file];
      else if (!(Pawns(side) & SetMask(sqflip[side][A2] + file))) {
        defects++;
        if (!(Pawns(side) & SetMask(sqflip[side][A3] + file)))
          defects++;
      }
    }
  return defects;
}

/* last modified 07/20/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKnights() is used to evaluate knights.                            *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKnights(TREE * RESTRICT tree, int side) {
  uint64_t temp;
  int square, special, i, score_eg = 0, score_mg = 0;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First fold in centralization score from the piece/      *
 *  square table "nval".                                    *
 *                                                          *
 ************************************************************
 */
  for (temp = Knights(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    score_mg += nval[mg][side][square];
    score_eg += nval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate for "outposts" which is a knight that can't    *
 *  be driven off by an enemy pawn, and which is supported  *
 *  by a friendly pawn.                                     *
 *                                                          *
 *  If the enemy has NO minor to take this knight, then     *
 *  increase the bonus.                                     *
 *                                                          *
 ************************************************************
 */
    special = knight_outpost[side][square];
    if (special && !(mask_no_pattacks[enemy][square] & Pawns(enemy))) {
      if (pawn_attacks[enemy][square] & Pawns(side)) {
        special += special / 2;
        if (!Knights(enemy) && !(Color(square) & Bishops(enemy)))
          special += knight_outpost[side][square];
      }
      score_eg += special;
      score_mg += special;
    }
/*
 ************************************************************
 *                                                          *
 *  Mobility counts the number of squares the knight        *
 *  attacks, excluding squares with friendly pieces, and    *
 *  weighs each square according to centralization.         *
 *                                                          *
 ************************************************************
 */
    score_mg += knight_mobility_table[square];
    score_eg += knight_mobility_table[square];
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      i = Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_n[i];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 08/07/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMate() is used to evaluate positions where neither side has pawns *
 *   and one side has enough material to force checkmate.  It simply trys to   *
 *   force the losing king to the edge of the board, and then to the corner    *
 *   where mates are easier to find.                                           *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateMate(TREE * RESTRICT tree, int side) {
  int mate_score = 0;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  If one side has a bishop+knight and the other side has  *
 *  no pieces or pawns, then use the special bishop_knight  *
 *  scoring board for the losing king to force it to the    *
 *  right corner for mate.                                  *
 *                                                          *
 ************************************************************
 */
  if (!TotalPieces(enemy, occupied) && TotalMinors(side) == 2 &&
      TotalPieces(side, bishop) == 1) {
    if (dark_squares & Bishops(side))
      mate_score = b_n_mate_dark_squares[KingSQ(enemy)];
    else
      mate_score = b_n_mate_light_squares[KingSQ(enemy)];
  }
/*
 ************************************************************
 *                                                          *
 *  If one side is winning, force the enemy king to the     *
 *  edge of the board.                                      *
 *                                                          *
 ************************************************************
 */
  else {
    mate_score = mate[KingSQ(enemy)];
    mate_score -=
        (Distance(KingSQ(side), KingSQ(enemy)) - 3) * king_king_tropism;
  }
  tree->score_eg += sign[side] * mate_score;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *  EvaluateMaterial() is used to evaluate material on the board.  It really   *
 *  accomplishes detecting cases where one side has made a 'bad trade' as the  *
 *  comments below show.                                                       *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateMaterial(TREE * RESTRICT tree, int wtm) {
  int score_mg, score_eg, majors, minors;
  const int bad_trade = 90;

/*
 **********************************************************************
 *                                                                    *
 *  We start with the raw Material balance for the current position,  *
 *  then adjust this with a small bonus for the side on move.         *
 *                                                                    *
 **********************************************************************
 */
  score_mg = Material + ((wtm) ? wtm_bonus[mg] : -wtm_bonus[mg]);
  score_eg = Material + ((wtm) ? wtm_bonus[eg] : -wtm_bonus[eg]);
/*
 **********************************************************************
 *                                                                    *
 *   test 1.  if Majors or Minors are not balanced, then if one side  *
 *   is only an exchange up or down, we do not give any sort of bad   *
 *   trade penalty/bonus.                                             *
 *                                                                    *
 *   test 2.  if Majors or Minors are not balanced, then if one side  *
 *   has more piece material points than the other (using normal      *
 *   piece values of 3, 3, 5, 9 for N, B, R and Q) then the side that *
 *   is behind in piece material gets a penalty.                      *
 *                                                                    *
 **********************************************************************
 */
  majors =
      TotalPieces(white, rook) + 2 * TotalPieces(white,
      queen) - TotalPieces(black, rook) - 2 * TotalPieces(black, queen);
  minors =
      TotalPieces(white, knight) + TotalPieces(white,
      bishop) - TotalPieces(black, knight) - TotalPieces(black, bishop);
  if (majors || minors)
    if (Abs(TotalPieces(white, occupied) - TotalPieces(black, occupied)) != 2
        && TotalPieces(white, occupied) - TotalPieces(black, occupied) != 0) {
      score_mg +=
          Sign(TotalPieces(white, occupied) - TotalPieces(black,
              occupied)) * bad_trade;
      score_eg +=
          Sign(TotalPieces(white, occupied) - TotalPieces(black,
              occupied)) * bad_trade;
    }
/*
 ************************************************************
 *                                                          *
 *  Add a bonus per side if side has a pair of bishops,     *
 *  which can become very strong in open positions.         *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, bishop) > 1) {
    score_mg += bishop_pair[mg];
    score_eg += bishop_pair[eg];
  }
  if (TotalPieces(black, bishop) > 1) {
    score_mg -= bishop_pair[mg];
    score_eg -= bishop_pair[eg];
  }
  tree->score_mg += score_mg;
  tree->score_eg += score_eg;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  This code considers pieces as well, so it has been         *
 *   separated from the normal EvaluatePawns() code that hashes information    *
 *   based only on pawn positions.                                             *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePassedPawns(TREE * RESTRICT tree, int side, int wtm) {
  uint64_t behind;
  int file, square, score_mg = 0, score_eg = 0;
  int pawns, rank;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  for (pawns = tree->pawn_score.passed[side]; pawns; pawns &= pawns - 1) {
    file = LSB8Bit(pawns);
    square = Advanced(side, Pawns(side) & file_mask[file]);
    rank = rankflip[side][Rank(square)];
/*
 ************************************************************
 *                                                          *
 *  We have located the most advanced pawn on this file,    *
 *  which is the only one that will get any sort of bonus.  *
 *  Add in the MG/EG scores first.                          *
 ************************************************************
 */
    score_mg += passed_pawn_value[mg][rank];
    score_eg += passed_pawn_value[eg][rank];
/*
 ************************************************************
 *                                                          *
 *  Add in a bonus if the passed pawn is connected with     *
 *  another pawn for support.                               *
 *                                                          *
 ************************************************************
 */
    if (mask_pawn_connected[square] & Pawns(side)) {
      score_mg += passed_pawn_connected[mg][rank];
      score_eg += passed_pawn_connected[eg][rank];
    }
/*
 ************************************************************
 *                                                          *
 *  See if this pawn is either supported by a friendly rook *
 *  from behind, or is attacked by an enemy rook from       *
 *  behind.  Note that doubled rooks will not get this      *
 *  bonus added in twice since only one directly attacks    *
 *  the pawn in question.                                   *
 *                                                          *
 ************************************************************
 */
    if (Rooks(white) | Rooks(black)) {
      behind =
          ((side) ? minus8dir[square] : plus8dir[square]) &
          RookAttacks(square, OccupiedSquares);
      if (behind) {
        if (behind & Rooks(side)) {
          score_mg += rook_behind_passed_pawn[mg][rank];
          score_eg += rook_behind_passed_pawn[eg][rank];
        } else if (behind & Rooks(enemy)) {
          score_mg -= rook_behind_passed_pawn[mg][rank];
          score_eg -= rook_behind_passed_pawn[eg][rank];
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  If the pawn is blockaded by an enemy piece, it cannot   *
 *  move and is therefore not nearly as valuable as if it   *
 *  were free to advance.  If it is blocked by a friendly   *
 *  piece, it is penalized 1/2 the normal blockade amount   *
 *  to encourage the blocking piece to move so the pawn can *
 *  advance.                                                *
 *                                                          *
 ************************************************************
 */
    if (OccupiedSquares & ((side) ? plus8dir[square] : minus8dir[square])) {
      score_eg -= passed_pawn_obstructed[rank];
      if (SetMask(square + direction[side]) & Occupied(enemy)) {
        score_mg -= passed_pawn_blockaded_by_enemy[mg][rank];
        score_eg -= passed_pawn_blockaded_by_enemy[eg][rank];
      } else if (SetMask(square + direction[side]) & Occupied(side)) {
        score_mg -= passed_pawn_blockaded_by_friendly[mg][rank];
        score_eg -= passed_pawn_blockaded_by_friendly[eg][rank];
      }
/*
 ************************************************************
 *                                                          *
 *  If the pawn has nothing blocking its progress to the    *
 *  promotion square, then we give a bonus.  If the pawn is *
 *  outside the square of the enemy king, this bonus is     *
 *  increased, because this makes it more difficult for the *
 *  opponent to trade pieces which might let this pawn run. *
 *                                                          *
 ************************************************************
 */
    } else {
      if (!(pawn_race[side][wtm][square] & Kings(enemy)))
        score_eg += passed_pawn_far_away[rank];
      else
        score_eg += passed_pawn_not_far_away[rank];
    }
/*
 ************************************************************
 *                                                          *
 *  Add in a bonus based on how close the friendly king is, *
 *  and a penalty based on how close the enemy king is.     *
 *  The bonus/penalty is based on how advanced the pawn is  *
 *  to attract the kings toward the most advanced (and most *
 *  dangerous) passed pawn.                                 *
 *                                                          *
 ************************************************************
 */
    square += direction[side];
    score_eg -=
        (Distance(square, KingSQ(side)) - Distance(square,
            KingSQ(enemy))) * pp_dist_bonus[rank];
  }
/*
 ************************************************************
 *                                                          *
 *  Check to see if side has an outside passed pawn.        *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.passed[side]
      && is_outside[tree->pawn_score.passed[side]]
      [tree->pawn_score.all[enemy]]) {
    score_mg += outside_passed[mg];
    score_eg += outside_passed[eg];
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawnRaces() is used to evaluate passed pawns when one       *
 *   side has passed pawns and the other side (or neither) has pieces.  In     *
 *   such a case, the critical question is can the defending king stop the pawn*
 *   from queening or is it too far away?  If only one side has pawns that can *
 *   "run" then the situation is simple.  When both sides have pawns that can  *
 *   "run" it becomes more complex as it then becomes necessary to see if      *
 *   one side can use a forced king move to stop the other side, while the     *
 *   other side doesn't have the same ability to stop ours.                    *
 *                                                                             *
 *   In the case of king and pawn endings with exactly one pawn, the simple    *
 *   evaluation rules are used:  if the king is two squares in front of the    *
 *   pawn then it is a win, if the king is one one square in front with the    *
 *   opposition, then it is a win,  if the king is on the 6th rank with the    *
 *   pawn close by, it is a win.  Rook pawns are handled separately and are    *
 *   more difficult to queen because the king can get trapped in front of the  *
 *   pawn blocking promotion.                                                  *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePassedPawnRaces(TREE * RESTRICT tree, int wtm) {
  int file, square;
  int queen_distance;
  int pawnsq;
  uint64_t pawns;
  int passed;
  int side, enemy;
  int queener[2] = { 8, 8 };
  int forced_km[2] = { 0, 0 };
/*
 ************************************************************
 *                                                          *
 *  Check to see if side has one pawn and neither side has  *
 *  any pieces.  If so, use the simple pawn evaluation      *
 *  logic.                                                  *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++) {
    enemy = Flip(side);
    if (Pawns(side) && !Pawns(enemy) && TotalPieces(white, occupied) == 0 &&
        TotalPieces(black, occupied) == 0) {
      for (pawns = Pawns(side); pawns; pawns &= pawns - 1) {
        pawnsq = LSB(pawns);
/*
 ************************************************************
 *                                                          *
 *  King must be in front of the pawn or we go no further.  *
 *                                                          *
 ************************************************************
 */
        if (sign[side] * Rank(KingSQ(side)) <= sign[side] * Rank(pawnsq))
          continue;
/*
 ************************************************************
 *                                                          *
 *  First a special case.  If this is a rook pawn, then the *
 *  king must be on the adjacent file, and be closer to the *
 *  queening square than the opposing king.                 *
 *                                                          *
 ************************************************************
 */
        if (File(pawnsq) == FILEA) {
          if (File(KingSQ(side)) == FILEB &&
              Distance(KingSQ(side),
                  sqflip[side][A8]) < Distance(KingSQ(enemy),
                  sqflip[side][A8])) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          continue;
        } else if (File(pawnsq) == FILEH) {
          if (File(KingSQ(side)) == FILEG &&
              Distance(KingSQ(side),
                  sqflip[side][H8]) < Distance(KingSQ(enemy),
                  sqflip[side][H8])) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          continue;
        }
/*
 ************************************************************
 *                                                          *
 *  If king is two squares in front of the pawn then it's a *
 *  win immediately.  If the king is on the 6th rank and    *
 *  closer to the pawn than the opposing king, it's also a  *
 *  win.                                                    *
 *                                                          *
 ************************************************************
 */
        if (Distance(KingSQ(side), pawnsq) < Distance(KingSQ(enemy), pawnsq)) {
          if (sign[side] * Rank(KingSQ(side)) >
              sign[side] * (Rank(pawnsq) - 1 + 2 * side)) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          if (Rank(KingSQ(side)) == rank6[side]) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
/*
 ************************************************************
 *                                                          *
 *  Last chance:  if the king is one square in front of the *
 *  pawn and has the opposition, then it's still a win.     *
 *                                                          *
 ************************************************************
 */
          if (Rank(KingSQ(side)) == Rank(pawnsq) - 1 + 2 * side &&
              EvaluateHasOpposition(wtm == side, KingSQ(side),
                  KingSQ(enemy))) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Check to see if enemy is out of pieces and stm has      *
 *  passed pawns.  If so, see if any of these passed pawns  *
 *  can outrun the defending king and promote.              *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(enemy, occupied) == 0 && tree->pawn_score.passed[side]) {
      passed = tree->pawn_score.passed[side];
      for (; passed; passed &= passed - 1) {
        file = LSB8Bit(passed);
        square = Advanced(side, Pawns(side) & file_mask[file]);
        forced_km[enemy] =
            (pawn_race[side][wtm][square] & Kings(enemy)) !=
            (pawn_race[side][Flip(wtm)][square] & Kings(enemy));
        if (!(pawn_race[side][wtm][square] & Kings(enemy))) {
          queen_distance = Abs(rank8[side] - Rank(square));
          if (Kings(side) & ((side) ? plus8dir[square] : minus8dir[square])) {
            if (file == FILEA || file == FILEH)
              queen_distance = 99;
            queen_distance++;
          }
          if (Rank(square) == rank2[side])
            queen_distance--;
          if (queen_distance < queener[side])
            queener[side] = queen_distance;
        }
      }
    }
  }
  if (queener[white] == 8 && queener[black] == 8)
    return;
/*
 ************************************************************
 *                                                          *
 *  Now that we know which pawns can outrun the kings for   *
 *  each side, we need to do the following:                 *
 *                                                          *
 *    (1) If both sides are forced to move their king to    *
 *        prevent the opponent from promoting, we let the   *
 *        search resolve this as the depth increases.       *
 *                                                          *
 *    (2) If white can run while black can not, then white  *
 *        wins, or vice-versa.                              *
 *                                                          *
 *    (3) If white queens and black's king can't stop it    *
 *        no matter who moves first, while black has a pawn *
 *        that white can stop if a king move is made        *
 *        immediately, then white wins, and vice-versa.     *
 *                                                          *
 *    (4) Other situations are left to the search to        *
 *        resolve.                                          *
 *                                                          *
 ************************************************************
 */
  if (forced_km[white] & forced_km[black])
    return;
  if (queener[white] < 8 && queener[black] == 8) {
    tree->score_eg += pawn_can_promote + (5 - queener[white]) * 10;
    return;
  } else if (queener[black] < 8 && queener[white] == 8) {
    tree->score_eg += -(pawn_can_promote + (5 - queener[black]) * 10);
    return;
  }
  if (queener[white] < queener[black] && forced_km[white]) {
    tree->score_eg += pawn_can_promote + (5 - queener[white]) * 10;
    return;
  } else if (queener[black] < queener[white] && forced_km[white]) {
    tree->score_eg += -(pawn_can_promote + (5 - queener[black]) * 10);
    return;
  }
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePawns() is used to evaluate pawns.  It evaluates pawns for only   *
 *   one side, and fills in the pawn hash entry information.  It requires two  *
 *   calls to evaluate all pawns on the board.  Comments below indicate the    *
 *   particular pawn structure features that are evaluated.                    *
 *                                                                             *
 *   This procedure also fills in information (without scoring) that other     *
 *   evaluation procedures use, such as which pawns are passed or candidates,  *
 *   which pawns are weak, which files are open, and so forth.                 *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePawns(TREE * RESTRICT tree, int side) {
  uint64_t pawns;
  uint64_t temp;
  uint64_t p_moves[2];
  int square, file, rank, score_eg = 0, score_mg = 0;
  int defenders, attackers, sq;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.all[side] = 0;
  tree->pawn_score.passed[side] = 0;
/*
 ************************************************************
 *                                                          *
 *  First, determine which squares pawns can reach.         *
 *                                                          *
 ************************************************************
 */
  p_moves[side] = 0;
  for (pawns = Pawns(side); pawns; pawns &= pawns - 1) {
    square = LSB(pawns);
    tree->pawn_score.all[side] |= 1 << File(square);
    for (sq = square; sq != File(square) + (rank7[side] << 3);
        sq += direction[side]) {
      p_moves[side] |= SetMask(sq);
      if (SetMask(sq + direction[side]) & tree->all_pawns)
        break;
      defenders =
          PopCnt(pawn_attacks[enemy][sq + direction[side]] & Pawns(side));
      attackers =
          PopCnt(pawn_attacks[side][sq + direction[side]] & Pawns(enemy));
      if (attackers - defenders > 0)
        break;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Loop through all pawns for this side.                   *
 *                                                          *
 ************************************************************
 */
  for (pawns = Pawns(side); pawns; pawns &= pawns - 1) {
    square = LSB(pawns);
    file = File(square);
    rank = rankflip[side][Rank(square)];
/*
 ************************************************************
 *                                                          *
 *  Evaluate pawn advances.  Center pawns are encouraged to *
 *  advance, while wing pawns are pretty much neutral.      *
 *  This is a simple piece/square value.                    *
 *                                                          *
 ************************************************************
 */
    score_mg += pval[mg][side][square];
    score_eg += pval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate isolated pawns, which are penalized based on   *
 *  whether they are isolated, and whether they are on a    *
 *  half-open file making them easier to attack.            *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_isolated[square] & Pawns(side))) {
      score_mg -= pawn_isolated[mg];
      score_eg -= pawn_isolated[eg];
      if (!(Pawns(enemy) & file_mask[file])) {
        score_mg -= pawn_isolated[mg] / 2;
        score_eg -= pawn_isolated[eg] / 2;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Evaluate weak pawns.  Weak pawns are evaluated by the   *
 *  following rules:  (1) if a pawn is defended by a pawn,  *
 *  it isn't weak;  (2) if a pawn is undefended by a pawn   *
 *  and advances one (or two if it hasn't moved yet) ranks  *
 *  and is defended fewer times than it is attacked, it is  *
 *  weak.  Note that the penalty is greater if the pawn is  *
 *  on an open file.  Note that an isolated pawn is just    *
 *  another case of a weak pawn, since it can never be      *
 *  defended by a pawn.                                     *
 *                                                          *
 *  First, test the pawn where it sits to determine if it   *
 *  is defended more times than attacked.  If so, it is not *
 *  weak and we are done.  If it is weak where it sits, can *
 *  it advance one square and become not weak.  If so we    *
 *  are again finished with this pawn.  Otherwise we fall   *
 *  into the next test.                                     *
 *                                                          *
 ************************************************************
 */
    else {
      do {
        attackers = 0;
        defenders = 0;
        temp =
            p_moves[side] & ((side) ? plus8dir[square] : minus8dir[square]);
        for (; temp; temp &= temp - 1) {
          sq = LSB(temp);
          defenders = PopCnt(pawn_attacks[enemy][sq] & Pawns(side));
          attackers = PopCnt(pawn_attacks[side][sq] & Pawns(enemy));
          if (defenders && defenders >= attackers)
            break;
        }
        if (defenders && defenders >= attackers)
          break;
/*
 ************************************************************
 *                                                          *
 *  If the pawn can be defended by a pawn, and that pawn    *
 *  can safely advance to actually defend this pawn, then   *
 *  this pawn is not weak.                                  *
 *                                                          *
 ************************************************************
 */
        if (!(pawn_attacks[enemy][square] & p_moves[side])) {
          score_mg -= pawn_weak[mg];
          score_eg -= pawn_weak[eg];
          if (!(Pawns(enemy) & file_mask[file]))
            score_mg -= pawn_weak[mg] / 2;
        }
      } while (0);
/*
 ************************************************************
 *                                                          *
 *  Evaluate doubled pawns.  If there are other pawns on    *
 *  this file, penalize this pawn.                          *
 *                                                          *
 ************************************************************
 */
      if (PopCnt(file_mask[file] & Pawns(side)) > 1) {
        score_mg -= doubled_pawn_value[mg];
        score_eg -= doubled_pawn_value[eg];
      }
/*
 ************************************************************
 *                                                          *
 *  Test the pawn to see if it is connected with a neighbor *
 *  which makes it more mobile.                             *
 *                                                          *
 ************************************************************
 */
      if (mask_pawn_connected[square] & Pawns(side)) {
        score_mg += pawn_connected[mg];
        score_eg += pawn_connected[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Discover and flag passed pawns for use later.           *
 *                                                          *
 ************************************************************
 */
    if (!(mask_passed[side][square] & Pawns(enemy)))
      tree->pawn_score.passed[side] |= 1 << file;
/*
 ************************************************************
 *                                                          *
 *  Determine if this pawn is a candidate passer, since we  *
 *  now know it isn't passed.  A candidate is a pawn on a   *
 *  file with no enemy pawns in front of it, and if it      *
 *  advances until it contacts an enemy pawn, and it is     *
 *  defended as many times as it is attacked when it        *
 *  reaches that pawn, then all that is left is to see if   *
 *  it is passed when the attacker(s) get removed.          *
 *                                                          *
 ************************************************************
 */
    else if (!(file_mask[File(square)] & Pawns(enemy))
        && mask_pawn_isolated[square] & Pawns(side)
        && !(pawn_attacks[side][square] & Pawns(enemy))) {
      attackers = 1;
      defenders = 0;
      for (sq = square; sq != File(square) + (rank7[side] << 3);
          sq += direction[side]) {
        if (SetMask(sq + direction[side]) & tree->all_pawns)
          break;
        defenders = PopCnt(pawn_attacks[enemy][sq] & p_moves[side]);
        attackers = PopCnt(pawn_attacks[side][sq] & Pawns(enemy));
        if (attackers)
          break;
      }
      if (attackers <= defenders) {
        if (!(mask_passed[side][sq + direction[side]] & Pawns(enemy))) {
          score_mg += passed_pawn_candidate[mg][rank];
          score_eg += passed_pawn_candidate[eg][rank];
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Evaluate "hidden" passed pawns.  Simple case is a pawn  *
 *  chain (white) at b5, a6, with a black pawn at a7.       *
 *  It appears the b-pawn is backward, with a ram at a6/a7  *
 *  but this is misleading, because the pawn at a6 is       *
 *  really passed when white plays b6.                      *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == rank6[side]
        && SetMask(square + direction[side]) & Pawns(enemy)
        && ((File(square) < FILEH &&
                SetMask(square + 9 - 16 * side) & Pawns(side)
                && !(mask_hidden_right[side][File(square)] & Pawns(enemy)))
            || (File(square) > FILEA &&
                SetMask(square + 7 - 16 * side) & Pawns(side)
                && !(mask_hidden_left[side][File(square)] & Pawns(enemy))))) {
      score_mg += passed_pawn_hidden[mg];
      score_eg += passed_pawn_hidden[eg];
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Evaluate king safety.                                   *
 *                                                          *
 *  This uses the function EvaluateKingsFile() and looks at *
 *  four possible positions for the king, either castled    *
 *  kingside, queenside or else standing on the d or e file *
 *  stuck in the middle.  This essentially is about the     *
 *  pawns in front of the king and what kind of "shelter"   *
 *  they provide for the king during the middlegame.        *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.defects_q[side] = EvaluateKingsFile(tree, FILEB, side);
  tree->pawn_score.defects_d[side] = EvaluateKingsFile(tree, FILED, side);
  tree->pawn_score.defects_e[side] = EvaluateKingsFile(tree, FILEE, side);
  tree->pawn_score.defects_k[side] = EvaluateKingsFile(tree, FILEG, side);
  tree->pawn_score.score_mg += sign[side] * score_mg;
  tree->pawn_score.score_eg += sign[side] * score_eg;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateQueens() is used to evaluate queens.                              *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateQueens(TREE * RESTRICT tree, int side) {
  uint64_t temp;
  int square, i, score_mg = 0, score_eg = 0;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First locate each queen and obtain it's centralization  *
 *  score from the static piece/square table for queens.    *
 *                                                          *
 ************************************************************
 */
  for (temp = Queens(side); temp; temp &= temp - 1) {
    square = LSB(temp);
/*
 ************************************************************
 *                                                          *
 *  Then, add in the piece/square table value for the       *
 *  queen.                                                  *
 *                                                          *
 ************************************************************
*/
    score_mg += qval[mg][side][square];
    score_eg += qval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      i = KingSQ(enemy);
      tree->tropism[side] += king_tropism_q[Distance(square, i)];
      i = 8 - (RankDistance(square, i) + FileDistance(square, i));
      score_mg += i;
      score_eg += i;
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 08/20/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateRooks() is used to evaluate rooks.                                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateRooks(TREE * RESTRICT tree, int side) {
  uint64_t temp, moves;
  int square, rank, file, i, mobility;
  int score_mg = 0, score_eg = 0;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  for (temp = Rooks(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    file = File(square);
    rank = Rank(square);
/*
 ************************************************************
 *                                                          *
 *  Determine if the rook is on an open file or on a half-  *
 *  open file, either of which increases its ability to     *
 *  attack important squares.                               *
 *                                                          *
 ************************************************************
 */
    if (!(file_mask[file] & Pawns(side))) {
      if (!(file_mask[file] & Pawns(enemy))) {
        score_mg += rook_open_file[mg];
        score_eg += rook_open_file[eg];
      } else {
        score_mg += rook_half_open_file[mg];
        score_eg += rook_half_open_file[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Check to see if the king has been forced to move and    *
 *  has trapped a rook at a1/b1/g1/h1, if so, then penalize *
 *  the trapped rook to help extricate it.                  *
 *                                                          *
 ************************************************************
 */
    if (rank == rank1[side] && rank == Rank(KingSQ(side))) {
      i = File(KingSQ(side));
      if (i > FILEE) {
        if (file > i) {
          score_mg -= rook_trapped;
          score_eg -= rook_trapped;
        }
      } else if (i < FILED && file < i) {
        score_mg -= rook_trapped;
        score_eg -= rook_trapped;
      }
    }
/*
 ************************************************************
 *                                                          *
 *   finally check to see if any rooks are on the 7th rank, *
 *   with the opponent having pawns on that rank or the     *
 *   opponent's king being hemmed in on the 7th/8th rank.   *
 *   If so, we give a bonus for the strong rook.  If there  *
 *   is another rook on the 7th that is connected with this *
 *   one, then the positional advantage is even stronger.   *
 *                                                          *
 ************************************************************
 */
    else if (rank == rank7[side] && (Rank(KingSQ(enemy)) == rank8[side]
            || Pawns(enemy) & rank_mask[rank7[side]])) {
      score_mg += rook_on_7th[mg];
      score_eg += rook_on_7th[eg];
      if (RankAttacks(square) & Rooks(side)) {
        score_mg += rook_connected_7th[mg];
        score_eg += rook_connected_7th[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Mobility counts the number of squares the rook attacks, *
 *  excluding squares with friendly pieces, and weighs each *
 *  square according to a complex formula that includes     *
 *  files as well as total number of squares attacked.      *
 *                                                          *
 ************************************************************
 */
    mobility = RookMobility(square, OccupiedSquares);
    score_mg += mobility;
    score_eg += mobility;

/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      moves = king_attacks[KingSQ(enemy)];
      i = (rook_attacks[square] & moves &&
          RookAttacks(square,
              OccupiedSquares & ~(Queens(side) | Rooks(side))) & moves) ? 1 :
          Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_r[i];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 08/20/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateWinningChances() is used to determine if one side has reached a   *
 *   position which can not be won, period, even though side may be ahead in   *
 *   material in some way.                                                     *
 *                                                                             *
 *   Return values:                                                            *
 *        0    ->     side on move can not win.                                *
 *        1    ->     side on move can win.                                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateWinningChances(TREE * RESTRICT tree, int side, int wtm) {
  int square, ekd, promote, majors, minors;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  If neither side has any pieces, and both sides have     *
 *  non-rookpawns, then either side can win.                *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, occupied) == 0 && TotalPieces(black, occupied) == 0
      && Pawns(white) & not_rook_pawns && Pawns(black) & not_rook_pawns)
    return 1;
/*
 ************************************************************
 *                                                          *
 *  If one side is an exchange up, but has no pawns, then   *
 *  that side can not possibly win.                         *
 *                                                          *
 ************************************************************
 */
  majors = TotalMajors(side) - TotalMajors(enemy);
  if (Abs(majors) == 1) {
    minors = TotalMinors(enemy) - TotalMinors(side);
    if (majors == minors)
      if (TotalPieces(side, pawn) == 0)
        return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  If side has a piece and no pawn, it can not possibly    *
 *  win.  If side is a piece ahead, the only way it can win *
 *  is if the enemy is already trapped on the edge of the   *
 *  board (special case to handle KRB vs KR which can be    *
 *  won if the king gets trapped).                          *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 0) {
    if (TotalPieces(side, occupied) <= 3)
      return 0;
    if (TotalPieces(side, occupied) - TotalPieces(enemy, occupied) <= 3 &&
        mask_not_edge & Kings(enemy))
      return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  If "side" has a pawn, then either the pawn had better   *
 *  not be a rook pawn, or else side had better have the    *
 *  right color bishop or any other piece, otherwise it is  *
 *  not winnable if the enemy king can get to the queening  *
 *  square first.                                           *
 *                                                          *
 ************************************************************
 */
  else if (!(Pawns(side) & not_rook_pawns))
    do {
      if (TotalPieces(side, occupied) > 3 || (TotalPieces(side, occupied) == 3
              && Knights(side)))
        continue;
      if (file_mask[FILEA] & Pawns(side) && file_mask[FILEH] & Pawns(side))
        continue;
      if (Bishops(side)) {
        if (Bishops(side) & dark_squares) {
          if (file_mask[dark_corner[side]] & Pawns(side))
            continue;
        } else if (file_mask[light_corner[side]] & Pawns(side))
          continue;
      }
      if (Pawns(side) & file_mask[FILEA])
        promote = A8;
      else
        promote = H8;
      ekd = Distance(KingSQ(enemy), sqflip[side][promote]) - (wtm != side);
      if (ekd <= 1)
        return 0;
    } while (0);
/*
 ************************************************************
 *                                                          *
 *  Check to see if this is a KRP vs KR ending.  If so, and *
 *  the losing king is in front of the passer, then this is *
 *  a drawish ending.                                       *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 1 && TotalPieces(enemy, pawn) == 0 &&
      TotalPieces(side, occupied) == 5 && TotalPieces(enemy, occupied) == 5) {
    square = LSB(Pawns(side));
    if (FileDistance(KingSQ(enemy), square) <= 1 &&
        InFront(side, Rank(KingSQ(enemy)), Rank(square)))
      return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  If this side has pawns, and we have made it through the *
 *  previous tests, then this side has winning chances.     *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn))
    return 1;
/*
 ************************************************************
 *                                                          *
 *  If this side has two bishops, and the enemy has only a  *
 *  single kinght, the two bishops win.                     *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, occupied) == 6)
    if (TotalPieces(enemy, occupied) == 3 && (Knights(side)
            || !Knights(enemy)))
      return 0;
/*
 ************************************************************
 *                                                          *
 *  If one side is two knights ahead and the opponent has   *
 *  no remaining material, it is a draw.                    *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, occupied) == 6 && !Bishops(side)
      && TotalPieces(enemy, occupied) + TotalPieces(enemy, pawn) == 0)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  If we make it through all the above tests, then "side"  *
 *  can win so we return 1;                                 *
 *                                                          *
 ************************************************************
 */
  return 1;
}
