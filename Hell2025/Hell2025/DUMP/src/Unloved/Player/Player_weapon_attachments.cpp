#include "Player.h"

#include "Hell/Math/Math.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Viewport/ViewportManager.h"

namespace Unloved {

void Player::UpdateWeaponAttachments() {
	SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
	SkinnedModel* skinnedModel = viewWeapon->GetSkinnedModel();

	if (!viewWeapon || Hell::Math::IsNan(viewWeapon->GetModelMatrix())) {
		return;
	}

    return;

	if (viewWeapon->GetSkinnedModel()->GetName() == "Glock") {
		{
			glm::mat4 globalBlendedNodeTransform = viewWeapon->GetNodeModelSpaceMatrix("Sight");
			glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Sight");
			glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();

			glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset;
			m_redDot.Update(finalMatrix);
			m_redDot.DrawWorldspaceAABBs(YELLOW);

			//const glm::mat4 modelMatrix = viewWeapon->GetBoneWorldMatrixWithBoneOffset("Sight");
			//m_redDot.Update(modelMatrix);
			RenderDataManager::SubmitRenderItems(m_redDot.GetRenderItems(), true);
		}
		{
			const glm::mat4 modelMatrix = viewWeapon->GetNodeWorldMatrix("Suppressor") * skinnedModel->GetBoneOffset("Suppressor");
            m_supressor.Update(modelMatrix);
            m_supressor.DrawWorldspaceAABBs(YELLOW);

            RenderDataManager::SubmitRenderItems(m_supressor.GetRenderItems(), true);
		}
	}


    return;


    if (viewWeapon->GetSkinnedModel()->GetName() == "P90") {
        Transform offset;
        offset.position = glm::vec3(0.0f, 0.0f, -1.000003f);
        glm::mat4 offsetMatrix = offset.to_mat4();

        glm::mat4 globalBlendedNodeTransform = viewWeapon->GetNodeModelSpaceMatrix("Magazine2");
        glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Magazine2");
        glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();
        glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset * offsetMatrix;
        //glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset;

        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform)[3], BLUE);
        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform * boneOffset)[3], WHITE);
        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform * boneOffset)[3] * offsetMatrix, ORANGE);

        m_p90MagMeshNodes.Update(finalMatrix);
        RenderDataManager::SubmitRenderItems(m_p90MagMeshNodes.GetRenderItems(), true);


        //m_p90MagTest.DrawWorldspaceAABBs(GREEN);
    }
}

} // namespace Unloved
